#include "adapter.hpp"
#include "builder.hpp"
#include "command.hpp"
#include "observer.hpp"

int main()
{
    try
    {
        auto order = OrderBuilder("ORD-2026-001")
                         .addItem("C++ 图书", 2, 80.0, 10)
                         .addItem("机械键盘", 1, 300.0, 3)
                         .build();

        SmsObserver sms;
        LogObserver log;
        order->addObserver(sms);
        order->addObserver(log);

        std::unique_ptr<OrderValidator> validators(new NonEmptyValidator);
        validators->setNext(
            std::unique_ptr<OrderValidator>(new StockValidator));

        LegacyPaySdk sdk;
        LegacyPayAdapter payment(sdk);
        ShopService shop(*validators, payment);

        CheckoutCommand checkout(
            shop, *order, std::unique_ptr<PricingStrategy>(new VipPricing));
        checkout.execute();
        order->ship();

        // 已发货订单不能取消，下面演示 State 对非法操作的保护。
        try
        {
            checkout.undo();
        }
        catch (const std::logic_error &e)
        {
            std::cout << "[业务拒绝] " << e.what() << '\n';
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "处理失败：" << e.what() << '\n';
        return 1;
    }
}