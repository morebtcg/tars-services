#include "../Common/TarsUtils.h"
#include "ExecutorServiceServer.h"
#include <tarscpp/servant/Application.h>

using namespace bcostars;

class ExecutorServiceApp : public Application
{
public:
    ~ExecutorServiceApp() override{};

    void initialize() override
    {
        addAllConfig(this);
        addServant<ExecutorServiceServer>(ServerConfig::Application + "." +
                                          ServerConfig::ServerName + "." +
                                          bcos::protocol::EXECUTOR_SERVANT_NAME);
    }

    void destroyApp() override {}
};

int main(int argc, char* argv[])
{
    try
    {
        bcos::initializer::initCommandLine(argc, argv);
        ExecutorServiceApp app;
        app.main(argc, argv);
        app.waitForShutdown();

        return 0;
    }
    catch (std::exception& e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}