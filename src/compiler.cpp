#include "compiler.hpp"

#include "compile_error.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Program.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "util/fmt.hpp"


void pars::compile_exe(std::string_view output_path)
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();

	std::string error;

    auto target_triple = llvm::sys::getDefaultTargetTriple();
    auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);

	if (!target)
	{
		throw CompileError{nullptr, fmt::format("Target lookup failed: {}", error)};
	}

	std::error_code ec;

	llvm::TargetOptions opt;

	auto cpu = "generic";
	auto features = "";
	auto rm = llvm::Reloc::Model::PIC_;

	std::unique_ptr<llvm::TargetMachine> machine
	{
		target->createTargetMachine(target_triple, cpu, features, opt, rm)
	};

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

	std::vector<llvm::StringRef> linker_args =
	{
		"clang",
		"-o", output_path,
		"-no-pie",
		"-lm",
		// "-Wl,-e,_start"
	};

	auto obj_files_start = linker_args.size();

	for (auto *module : get_all_modules())
	{
		auto *llvm_module = module->module;

		// llvm_module->setTargetTriple(target_triple);
		llvm_module->setDataLayout(machine->createDataLayout());

		auto name = std::string{llvm_module->getName()};

		std::replace(name.begin(), name.end(), '/', '_');

		auto *obj_path = new std::string{name + ".o"};

		llvm::raw_fd_ostream obj_os (*obj_path, ec, llvm::sys::fs::OF_None);

		if (ec)
		{
			throw CompileError{nullptr, fmt::format("Cannot open output file: {}", ec.message())};
		}

		llvm::legacy::PassManager codegen_pm;

		mpm.run(*llvm_module, mam);

		machine->addPassesToEmitFile(codegen_pm, obj_os, nullptr,
								  llvm::CodeGenFileType::ObjectFile);

		codegen_pm.run(*llvm_module);

		linker_args.emplace_back(*obj_path);
	}

    std::string linker_error;

    auto result = llvm::sys::ExecuteAndWait
	(
        "/usr/bin/clang",  // path to linker
        linker_args,
        std::nullopt,        // env
        {},                // redirects
        0, 0, &linker_error
    );

	for (auto i = obj_files_start; i < linker_args.size(); i++)
	{
		llvm::sys::fs::remove(linker_args[i]);
	}

    if (result != 0)
    {
    	throw CompileError{nullptr, fmt::format("Linking failed: {}", linker_error)};
    }
}
