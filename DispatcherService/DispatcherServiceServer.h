#pragma once
#include "../Common/ErrorConverter.h"
#include "../Common/TarsUtils.h"
#include "../TxPoolService/TxPoolServiceClient.h"
#include "../libinitializer/ProtocolInitializer.h"
#include "../protocols/BlockImpl.h"
#include "DispatcherService.h"
#include "bcos-crypto/hash/SM3.h"
#include "bcos-crypto/signature/sm2/SM2Crypto.h"
#include "bcos-dispatcher/DispatcherImpl.h"
#include "servant/Application.h"
#include <bcos-framework/libtool/NodeConfig.h>
#include <bcos-framework/libutilities/BoostLogInitializer.h>
#include <memory>


#define DISPATCHERSERVICE_LOG(LEVEL) BCOS_LOG(LEVEL) << "[DISPATCHERSERVICE]"

namespace bcostars
{
class DispatcherServiceServer : public DispatcherService
{
public:
    void initialize() override
    {
        std::call_once(m_initFlag, [this]() {
            auto configPath = ServerConfig::BasePath + "config.ini";

            // init the log
            boost::property_tree::ptree pt;
            boost::property_tree::read_ini(configPath, pt);
            m_logInitializer = std::make_shared<bcos::BoostLogInitializer>();
            // set the boost log into the tars log directory
            m_logInitializer->setLogPath(getLogPath());
            m_logInitializer->initLog(pt);

            auto nodeConfig = std::make_shared<bcos::tool::NodeConfig>();
            nodeConfig->loadConfig(configPath);

            auto protocolInitializer = std::make_shared<bcos::initializer::ProtocolInitializer>();
            protocolInitializer->init(nodeConfig);

            m_blockFactory = protocolInitializer->blockFactory();
            m_dispatcher = std::make_shared<bcos::dispatcher::DispatcherImpl>();
            // set the txpool to the dispatcher
            auto txpoolProxy =
                Application::getCommunicator()->stringToProxy<bcostars::TxPoolServicePrx>(
                    getProxyDesc(TXPOOL_SERVICE_NAME));
            auto txpool = std::make_shared<bcostars::TxPoolServiceClient>(
                txpoolProxy, protocolInitializer->cryptoSuite());
            DISPATCHERSERVICE_LOG(INFO) << LOG_DESC("init and start the dispatcher service");
            m_dispatcher->init(txpool);
            m_dispatcher->start();
            DISPATCHERSERVICE_LOG(INFO)
                << LOG_DESC("init and start the dispatcher service success");
        });
    }

    void destroy() override {}

    bcostars::Error asyncExecuteBlock(const bcostars::Block& block, tars::Bool verify,
        bcostars::BlockHeader& blockHeader, tars::TarsCurrentPtr current) override
    {
        current->setResponse(false);

        auto bcosBlock = m_blockFactory->createBlock();
        std::dynamic_pointer_cast<bcostars::protocol::BlockImpl>(bcosBlock)->setInner(block);
        m_dispatcher->asyncExecuteBlock(bcosBlock, verify,
            [current](const bcos::Error::Ptr& error,
                const bcos::protocol::BlockHeader::Ptr& blockHeader) {
                async_response_asyncExecuteBlock(current, toTarsError(error),
                    std::dynamic_pointer_cast<bcostars::protocol::BlockHeaderImpl>(blockHeader)
                        ->inner());
            });
        return bcostars::Error();
    }

    bcostars::Error asyncGetLatestBlock(
        bcostars::Block& block, tars::TarsCurrentPtr current) override
    {
        current->setResponse(false);

        m_dispatcher->asyncGetLatestBlock(
            [current](const bcos::Error::Ptr& error, const bcos::protocol::Block::Ptr& block) {
                async_response_asyncGetLatestBlock(current, toTarsError(error),
                    std::dynamic_pointer_cast<bcostars::protocol::BlockImpl>(block)->inner());
            });

        return bcostars::Error();
    }

    bcostars::Error asyncNotifyExecutionResult(const bcostars::Error& error,
        const bcostars::BlockHeader& blockHeader, tars::TarsCurrentPtr current) override
    {
        current->setResponse(false);

        auto bcosBlockHeader = m_blockFactory->blockHeaderFactory()->createBlockHeader();
        std::dynamic_pointer_cast<bcostars::protocol::BlockHeaderImpl>(bcosBlockHeader)
            ->setInner(blockHeader);
        m_dispatcher->asyncNotifyExecutionResult(
            toBcosError(error), bcosBlockHeader, [current](const bcos::Error::Ptr& error) {
                async_response_asyncNotifyExecutionResult(current, toTarsError(error));
            });
        return bcostars::Error();
    }

private:
    static std::once_flag m_initFlag;
    static bcos::dispatcher::DispatcherImpl::Ptr m_dispatcher;
    static bcostars::protocol::BlockFactoryImpl::Ptr m_blockFactory;
    static bcos::BoostLogInitializer::Ptr m_logInitializer;
};

}  // namespace bcostars