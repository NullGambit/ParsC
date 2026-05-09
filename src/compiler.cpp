#include "compiler.hpp"

#include "compile_error.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"           // was llvm/Support/Host.h
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"           // was llvm/Support/TargetRegistry.h
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Program.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "util/fmt.hpp"

void pars::Compiler::visit(FnPrototypeStmt *fn)
{
	fn->emit(ctx);
}

void pars::Compiler::visit(BlockStmt *fn)
{
	fn->emit(ctx);
}

void pars::Compiler::visit(ExprFnStmt *fn)
{
	fn->emit(ctx);
}

void pars::compile_exe(EmitCtx &ctx, std::string_view output_path)
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();

	std::string error;

    auto target_triple = llvm::sys::getDefaultTargetTriple();

    ctx.module->setTargetTriple(target_triple);

    auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);

	std::error_code ec;

    if (!target)
    {
    	throw CompileError{nullptr, fmt::format("Target lookup failed: {}", error)};
    }

    llvm::TargetOptions opt;

    auto cpu = "generic";
    auto features = "";
    auto rm = llvm::Reloc::Model::PIC_;

    std::unique_ptr<llvm::TargetMachine> machine
	{
        target->createTargetMachine(target_triple, cpu, features, opt, rm)
    };

    ctx.module->setDataLayout(machine->createDataLayout());

    auto obj_path = std::string{output_path} + ".o";

    llvm::raw_fd_ostream obj_os (obj_path, ec, llvm::sys::fs::OF_None);

	llvm::LoopAnalysisManager lam;
	llvm::FunctionAnalysisManager fam;
	llvm::CGSCCAnalysisManager cgam;
	llvm::ModuleAnalysisManager mam;

	// Create PassBuilder with the TargetMachine
	llvm::PassBuilder pb(machine.get());

	pb.registerModuleAnalyses(mam);
	pb.registerFunctionAnalyses(fam);
	pb.registerCGSCCAnalyses(cgam);
	pb.registerLoopAnalyses(lam);
	pb.crossRegisterProxies(lam, fam, cgam, mam);

	auto mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O0);

	mpm.run(*ctx.module, mam);

	llvm::legacy::PassManager codegen_pm;

	machine->addPassesToEmitFile(codegen_pm, obj_os, nullptr,
								  llvm::CodeGenFileType::ObjectFile);

	codegen_pm.run(*ctx.module);

    if (ec)
    {
    	throw CompileError{nullptr, fmt::format("Cannot open output file: {}", ec.message())};
    }

    obj_os.flush();

    std::vector<llvm::StringRef> linker_args =
    {
        "clang",           // or "gcc", "ld", "lld"
        "-o", output_path,
        obj_path,
        "-no-pie",         // if you used static reloc model
        "-lm",
        // "-Wl,-e,_start"  // custom entry point (see below)
    };

    std::string linker_error;

    auto result = llvm::sys::ExecuteAndWait
	(
        "/usr/bin/clang",  // path to linker
        linker_args,
        std::nullopt,        // env
        {},                // redirects
        0, 0, &linker_error
    );

    if (result != 0)
    {
    	throw CompileError{nullptr, fmt::format("Linking failed: {}", linker_error)};
    }

    llvm::sys::fs::remove(obj_path);
}
