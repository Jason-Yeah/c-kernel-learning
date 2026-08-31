#include "composite.hpp"
#include "facade.hpp"
#include "interpreter.hpp"
#include "proxy.hpp"
#include "template_factory_method.hpp"
#include <iostream>

int main()
{
    try
    {
        FileTypeFactory types;
        auto cpp = types.get(".cpp", "source");
        auto log = types.get(".log", "log");
        // std::cout << types.typeCount() << std::endl;

        Directory root("server", true);
        auto source = std::unique_ptr<Directory>(new Directory("src"));
        source->add(std::unique_ptr<Node>(new File("main.cpp", 12, cpp)));
        source->add(std::unique_ptr<Node>(new File("worker.cpp", 25, cpp)));

        auto logs = std::unique_ptr<Directory>(new Directory("logs"));
        logs->add(std::unique_ptr<Node>(new File("access.log", 80, log)));
        logs->add(std::unique_ptr<Node>(new File("leak.log", 2048, log)));
        root.add(std::move(source));
        root.add(std::move(logs));

        //
        StatisticsVisitor statistics;
        root.accept(statistics);
        std::cout << "[统计] " << statistics.result()
                  << "，共享类型对象数=" << types.typeCount() << '\n';

        AndExpression riskyLog(
            std::unique_ptr<RuleExpression>(new ExtensionIs(".log")),
            std::unique_ptr<RuleExpression>(new SizeGreaterThan(1024)));
        std::cout << "[规则] " << riskyLog.describe() << '\n';

        // 
        SecureScanProxy proxy(
            std::unique_ptr<ScanService>(new RealScanService));
        SecurityCenter center(proxy);
        TextExporterCreator textCreator;
        JsonExporterCreator jsonCreator;

        try
        {
            center.audit(root, Role::Employee, riskyLog, textCreator);
        }
        catch (const std::exception &e)
        {
            std::cout << "[拒绝] " << e.what() << '\n';
        }

        std::cout << center.audit(root, Role::Admin, riskyLog, textCreator);
        std::cout << center.audit(root, Role::Admin, riskyLog, jsonCreator);
    }
    catch (const std::exception &e)
    {
        std::cerr << "审计失败：" << e.what() << '\n';
        return 1;
    }
}