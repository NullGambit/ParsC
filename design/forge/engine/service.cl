module forge.engine.service

trait Service 
{
    fn init(options: EngineOptions): error
    fn update()
    fn shutdown()
    fn is_critical(): bool
}