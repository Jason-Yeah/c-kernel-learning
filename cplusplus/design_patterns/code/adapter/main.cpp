#include "adapter.hpp"
#include "xmltarget.hpp"

class DataConsumer
{
public:
    void display(JsonDataProvider &provider)
    {
        std::cout << "客户端收到 JSON: " << provider.GetJson() << std::endl;
    }
};

int main()
{
    auto xmlParser =
        std::make_unique<XmlParser>("<user><name>张三</name></user>");

    // 用适配器包装成客户端能用的接口
    Xml2JsonAdapter adapter(std::move(xmlParser));

    // 客户端开心地使用——它完全不知道 XML 的存在
    DataConsumer consumer;
    consumer.display(adapter);

    return 0;
}
