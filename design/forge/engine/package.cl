module forge.engine

import std.collections

trait ServiceManager
{
    fn add_service<T: Service>(): *Service?
    fn get_service<T: Service>(): *Service?
    fn init(options: EngineOptions): error
    fn start()
    fn shutdown()
}

struct DefaultEngine : Drop, ServiceManager
{
    should_run: bool
    private services: List<*Service>
    private service_table: Map<Type, *Service>

    fn drop()
    {
        for service in services 
        {
            #free(service)
        }
    }

    fn add_service<T: Service>(): *Service? 
    {
        if service_table.contains(T)
        {
            return Error("Service already exists")
        }

        var service = #alloc<T>()

        services.add(service)
        service_table.add(T, service)

        return service
    }

    fn get_service<T: Service>() => service_table.get(T)

    // error types do not need to return and will by default be empty
    fn init(options: EngineOptions): error
    {
        for service in services 
        {
            service.init(options)?
        }
    }

    fn start()
    {
        while should_run 
        {
            for service in services 
            {
                service.update()
            }
        }
    }

    fn shutdown()
    {
        reverse_for service in services 
        {
            service.shutdown()
        }
    }
}

static struct Engine 
{
    instance: *ServiceManager 

    alias instance

    fn create_default()
    {
        instance = #alloc<DefaultEngine>()
    }
}