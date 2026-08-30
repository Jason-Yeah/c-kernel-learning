# 设计模式综合实战

前面的示例大多用于理解单个模式，现通过两个完整的小项目练习如何在同一个业务中组合多种模式。重点不是“使用的模式越多越好”，而是先识别变化点、对象职责和依赖方向，再选择恰当的模式。

示例均为单文件程序，使用 C++14 标准，可直接复制、编译和运行。

> **编译验证：** 两个示例已于 2026-08-30 在沙箱中使用 `g++ 13.3.0`，配合 `-std=c++14 -Wall -Wextra -pedantic` 选项从本文代码块直接提取、编译并运行。两者均以退出码 0 结束，未产生编译警告，实际输出与文中的预期输出一致。

---

## Demo 1：可扩展的电商订单处理系统

### 任务场景

一家电商平台需要实现从创建订单到发货的核心流程。系统当前支持普通价和会员价、第三方支付、下单前校验以及订单状态通知，后续还会增加新的优惠方式、支付渠道和通知渠道。

### 功能要求

1. 使用构建器逐步创建订单，避免带有大量参数的构造函数。
2. 下单前依次校验“购物车非空”和“库存是否充足”；任一环节失败都终止下单。
3. 普通用户按原价结算，VIP 用户享受九折；新增计价规则时不修改订单类。
4. 将已有的第三方支付 SDK 接入统一支付接口。
5. 订单只能按照 `草稿 -> 待支付 -> 已支付 -> 已发货` 的合法路径流转，也允许在发货前取消。
6. 状态发生变化时通知短信、日志等观察者。
7. 使用命令对象封装“结账”，并提供撤销操作；这里的撤销是业务补偿——取消订单，而不是简单回退内存字段。
8. 客户端只通过商城服务外观完成结账，不直接组织校验、计价和支付步骤。

### 模式分工

| 模式 | 在本例中的职责 | 主要变化点 |
|---|---|---|
| Builder | 构造包含多个订单项的订单 | 创建步骤 |
| Chain of Responsibility | 串联下单校验 | 校验规则及顺序 |
| Strategy | 计算最终价格 | 促销算法 |
| Adapter | 适配第三方支付 SDK | 外部接口 |
| State | 约束订单状态和行为 | 状态流转规则 |
| Observer | 广播订单状态变化 | 消息接收者 |
| Command | 封装结账及补偿操作 | 请求的执行方式 |
| Facade | 编排完整结账用例 | 子系统调用流程 |

### UML 与流程分析

#### 核心类图

下图省略了部分具体方法和字段，重点展示依赖方向。`ShopService` 只依赖校验器和支付接口；`Order` 分别持有可替换的计价策略和状态对象。

```mermaid
classDiagram
    direction LR

    class Order {
        -id: string
        -items: vector~OrderItem~
        -state: unique_ptr~OrderState~
        -pricing: unique_ptr~PricingStrategy~
        +submit()
        +pay()
        +ship()
        +cancel()
        +transitionTo(next)
    }

    class OrderBuilder {
        +addItem(name, quantity, price, stock) OrderBuilder
        +build() unique_ptr~Order~
    }

    class PricingStrategy {
        <<interface>>
        +calculate(original) double
    }
    class RegularPricing
    class VipPricing

    class OrderState {
        <<interface>>
        +name() string
        +submit(order)
        +pay(order)
        +ship(order)
        +cancel(order)
    }
    class DraftState
    class AwaitingPaymentState
    class PaidState
    class ShippedState
    class CancelledState

    class OrderObserver {
        <<interface>>
        +onStateChanged(orderId, state)
    }
    class SmsObserver
    class LogObserver

    class OrderValidator {
        <<abstract>>
        -next: unique_ptr~OrderValidator~
        +setNext(next) OrderValidator
        +validate(order)
        #check(order)
    }
    class NonEmptyValidator
    class StockValidator

    class PaymentGateway {
        <<interface>>
        +pay(orderId, amount) bool
    }
    class LegacyPayAdapter
    class LegacyPaySdk

    class ShopService {
        +checkout(order, pricing)
    }
    class Command {
        <<interface>>
        +execute()
        +undo()
    }
    class CheckoutCommand

    OrderBuilder ..> Order : creates
    Order *-- PricingStrategy : owns
    PricingStrategy <|.. RegularPricing
    PricingStrategy <|.. VipPricing
    Order *-- OrderState : owns
    OrderState <|.. DraftState
    OrderState <|.. AwaitingPaymentState
    OrderState <|.. PaidState
    OrderState <|.. ShippedState
    OrderState <|.. CancelledState
    Order o-- OrderObserver : notifies
    OrderObserver <|.. SmsObserver
    OrderObserver <|.. LogObserver
    OrderValidator o-- OrderValidator : next
    OrderValidator <|-- NonEmptyValidator
    OrderValidator <|-- StockValidator
    PaymentGateway <|.. LegacyPayAdapter
    LegacyPayAdapter --> LegacyPaySdk : adapts
    ShopService --> OrderValidator : validates with
    ShopService --> PaymentGateway : pays with
    ShopService --> Order : coordinates
    Command <|.. CheckoutCommand
    CheckoutCommand --> ShopService
    CheckoutCommand --> Order
```

图中 `*--` 表示组合/独占拥有，`o--` 表示聚合或非独占关联，`<|..` 表示接口实现，`-->` 和 `..>` 表示依赖。

#### 结账时序图

时序图展示一次成功结账的运行时协作。责任链内部可包含任意数量的校验器，而调用方只看到统一的 `validate()`。

```mermaid
sequenceDiagram
    autonumber
    actor Client as 客户端
    participant Cmd as CheckoutCommand
    participant Shop as ShopService
    participant Chain as ValidatorChain
    participant Order as Order
    participant State as OrderState
    participant Pay as PaymentGateway
    participant Obs as OrderObserver

    Client->>Cmd: execute()
    Cmd->>Shop: checkout(order, VipPricing)
    Shop->>Chain: validate(order)
    loop 每个校验器
        Chain->>Chain: check(order)
    end
    Shop->>Order: setPricing(VipPricing)
    Shop->>Order: payableAmount()
    Order-->>Shop: 414.00
    Shop->>Order: submit()
    Order->>State: submit(order)
    State->>Order: transitionTo(AwaitingPaymentState)
    Order-->>Obs: onStateChanged(待支付)
    Shop->>Pay: pay(orderId, 414.00)
    Pay-->>Shop: true
    Shop->>Order: pay()
    Order->>State: pay(order)
    State->>Order: transitionTo(PaidState)
    Order-->>Obs: onStateChanged(已支付)
    Shop-->>Cmd: checkout 完成
    Cmd-->>Client: execute 完成
```

#### 订单状态图

状态图是检查业务规则最直接的方式。终态“已发货”和“已取消”没有后续转换，因此对它们执行支付、发货或取消会由基类默认实现抛出异常。

```mermaid
stateDiagram-v2
    [*] --> 草稿: 创建订单
    草稿 --> 待支付: submit
    草稿 --> 已取消: cancel
    待支付 --> 已支付: pay 成功
    待支付 --> 已取消: cancel / 支付失败
    已支付 --> 已发货: ship
    已支付 --> 已取消: cancel / 退款补偿
    已发货 --> [*]
    已取消 --> [*]
```

### 可执行示例（C++14）

```cpp
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Order;

// ---------- Observer ----------
class OrderObserver {
public:
    virtual ~OrderObserver() = default;
    virtual void onStateChanged(const std::string& orderId,
                                const std::string& state) = 0;
};

class SmsObserver final : public OrderObserver {
public:
    void onStateChanged(const std::string& id,
                        const std::string& state) override {
        std::cout << "[短信] 订单 " << id << " 状态变为：" << state << '\n';
    }
};

class LogObserver final : public OrderObserver {
public:
    void onStateChanged(const std::string& id,
                        const std::string& state) override {
        std::cout << "[日志] order=" << id << ", state=" << state << '\n';
    }
};

// ---------- Strategy ----------
class PricingStrategy {
public:
    virtual ~PricingStrategy() = default;
    virtual double calculate(double original) const = 0;
};

class RegularPricing final : public PricingStrategy {
public:
    double calculate(double original) const override { return original; }
};

class VipPricing final : public PricingStrategy {
public:
    double calculate(double original) const override { return original * 0.9; }
};

// ---------- State ----------
class OrderState {
public:
    virtual ~OrderState() = default;
    virtual const char* name() const = 0;
    virtual void submit(Order&) const;
    virtual void pay(Order&) const;
    virtual void ship(Order&) const;
    virtual void cancel(Order&) const;
};

class DraftState final : public OrderState {
public:
    const char* name() const override { return "草稿"; }
    void submit(Order&) const override;
    void cancel(Order&) const override;
};

class AwaitingPaymentState final : public OrderState {
public:
    const char* name() const override { return "待支付"; }
    void pay(Order&) const override;
    void cancel(Order&) const override;
};

class PaidState final : public OrderState {
public:
    const char* name() const override { return "已支付"; }
    void ship(Order&) const override;
    void cancel(Order&) const override;
};

class ShippedState final : public OrderState {
public:
    const char* name() const override { return "已发货"; }
};

class CancelledState final : public OrderState {
public:
    const char* name() const override { return "已取消"; }
};

struct OrderItem {
    std::string name;
    int quantity;
    double unitPrice;
    int stock;
};

class Order {
public:
    Order(std::string id, std::vector<OrderItem> items)
        : id_(std::move(id)), items_(std::move(items)),
          state_(new DraftState), pricing_(new RegularPricing) {}

    const std::string& id() const { return id_; }
    const std::vector<OrderItem>& items() const { return items_; }
    const std::string stateName() const { return state_->name(); }

    double originalAmount() const {
        double result = 0.0;
        for (const auto& item : items_) {
            result += item.quantity * item.unitPrice;
        }
        return result;
    }

    double payableAmount() const { return pricing_->calculate(originalAmount()); }

    void setPricing(std::unique_ptr<PricingStrategy> strategy) {
        pricing_ = std::move(strategy);
    }

    void addObserver(OrderObserver& observer) { observers_.push_back(&observer); }

    void transitionTo(std::unique_ptr<OrderState> next) {
        state_ = std::move(next);
        for (auto* observer : observers_) {
            observer->onStateChanged(id_, state_->name());
        }
    }

    void submit() { state_->submit(*this); }
    void pay() { state_->pay(*this); }
    void ship() { state_->ship(*this); }
    void cancel() { state_->cancel(*this); }

private:
    std::string id_;
    std::vector<OrderItem> items_;
    std::unique_ptr<OrderState> state_;
    std::unique_ptr<PricingStrategy> pricing_;
    std::vector<OrderObserver*> observers_; // 观察者生命周期由客户端管理
};

void OrderState::submit(Order&) const { throw std::logic_error("当前状态不能提交"); }
void OrderState::pay(Order&) const { throw std::logic_error("当前状态不能支付"); }
void OrderState::ship(Order&) const { throw std::logic_error("当前状态不能发货"); }
void OrderState::cancel(Order&) const { throw std::logic_error("当前状态不能取消"); }

void DraftState::submit(Order& order) const {
    order.transitionTo(std::unique_ptr<OrderState>(new AwaitingPaymentState));
}
void DraftState::cancel(Order& order) const {
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}
void AwaitingPaymentState::pay(Order& order) const {
    order.transitionTo(std::unique_ptr<OrderState>(new PaidState));
}
void AwaitingPaymentState::cancel(Order& order) const {
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}
void PaidState::ship(Order& order) const {
    order.transitionTo(std::unique_ptr<OrderState>(new ShippedState));
}
void PaidState::cancel(Order& order) const {
    std::cout << "[退款] 原支付将在原渠道退回\n";
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}

// ---------- Builder ----------
class OrderBuilder {
public:
    explicit OrderBuilder(std::string id) : id_(std::move(id)) {}

    OrderBuilder& addItem(std::string name, int quantity,
                          double unitPrice, int stock) {
        items_.push_back({std::move(name), quantity, unitPrice, stock});
        return *this;
    }

    std::unique_ptr<Order> build() {
        return std::unique_ptr<Order>(new Order(std::move(id_), std::move(items_)));
    }

private:
    std::string id_;
    std::vector<OrderItem> items_;
};

// ---------- Chain of Responsibility ----------
class OrderValidator {
public:
    virtual ~OrderValidator() = default;

    OrderValidator& setNext(std::unique_ptr<OrderValidator> next) {
        next_ = std::move(next);
        return *next_;
    }

    void validate(const Order& order) const {
        check(order);
        if (next_) next_->validate(order);
    }

private:
    virtual void check(const Order&) const = 0;
    std::unique_ptr<OrderValidator> next_;
};

class NonEmptyValidator final : public OrderValidator {
    void check(const Order& order) const override {
        if (order.items().empty()) throw std::runtime_error("购物车不能为空");
    }
};

class StockValidator final : public OrderValidator {
    void check(const Order& order) const override {
        for (const auto& item : order.items()) {
            if (item.quantity <= 0 || item.quantity > item.stock) {
                throw std::runtime_error(item.name + " 库存不足或数量非法");
            }
        }
    }
};

// ---------- Adapter ----------
class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;
    virtual bool pay(const std::string& orderId, double amount) = 0;
};

class LegacyPaySdk {
public:
    int makePayment(const std::string& tradeNo, long cents) {
        std::cout << "[第三方支付] trade=" << tradeNo << ", cents=" << cents << '\n';
        return 0; // 第三方约定：0 表示成功
    }
};

class LegacyPayAdapter final : public PaymentGateway {
public:
    explicit LegacyPayAdapter(LegacyPaySdk& sdk) : sdk_(sdk) {}

    bool pay(const std::string& orderId, double amount) override {
        return sdk_.makePayment(orderId, static_cast<long>(amount * 100 + 0.5)) == 0;
    }

private:
    LegacyPaySdk& sdk_;
};

// ---------- Facade ----------
class ShopService {
public:
    ShopService(const OrderValidator& validator, PaymentGateway& gateway)
        : validator_(validator), gateway_(gateway) {}

    void checkout(Order& order, std::unique_ptr<PricingStrategy> pricing) {
        validator_.validate(order);
        order.setPricing(std::move(pricing));
        std::cout << std::fixed << std::setprecision(2)
                  << "原价：" << order.originalAmount()
                  << "，应付：" << order.payableAmount() << '\n';
        order.submit();
        if (!gateway_.pay(order.id(), order.payableAmount())) {
            order.cancel();
            throw std::runtime_error("支付失败");
        }
        order.pay();
    }

private:
    const OrderValidator& validator_;
    PaymentGateway& gateway_;
};

// ---------- Command ----------
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class CheckoutCommand final : public Command {
public:
    CheckoutCommand(ShopService& shop, Order& order,
                    std::unique_ptr<PricingStrategy> pricing)
        : shop_(shop), order_(order), pricing_(std::move(pricing)) {}

    void execute() override {
        if (executed_) throw std::logic_error("命令不能重复执行");
        shop_.checkout(order_, std::move(pricing_));
        executed_ = true;
    }

    void undo() override {
        if (!executed_) throw std::logic_error("尚未执行，不能撤销");
        order_.cancel();
        executed_ = false;
    }

private:
    ShopService& shop_;
    Order& order_;
    std::unique_ptr<PricingStrategy> pricing_;
    bool executed_ = false;
};

int main() {
    try {
        auto order = OrderBuilder("ORD-2026-001")
                         .addItem("C++ 图书", 2, 80.0, 10)
                         .addItem("机械键盘", 1, 300.0, 3)
                         .build();

        SmsObserver sms;
        LogObserver log;
        order->addObserver(sms);
        order->addObserver(log);

        std::unique_ptr<OrderValidator> validators(new NonEmptyValidator);
        validators->setNext(std::unique_ptr<OrderValidator>(new StockValidator));

        LegacyPaySdk sdk;
        LegacyPayAdapter payment(sdk);
        ShopService shop(*validators, payment);

        CheckoutCommand checkout(shop, *order,
                                 std::unique_ptr<PricingStrategy>(new VipPricing));
        checkout.execute();
        order->ship();

        // 已发货订单不能取消，下面演示 State 对非法操作的保护。
        try {
            checkout.undo();
        } catch (const std::logic_error& e) {
            std::cout << "[业务拒绝] " << e.what() << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "处理失败：" << e.what() << '\n';
        return 1;
    }
}
```

编译运行：

```bash
g++ -std=c++14 -Wall -Wextra -pedantic demo_order.cpp -o demo_order
./demo_order
```

预期输出（金额与主要事件）：

```text
原价：460.00，应付：414.00
[短信] 订单 ORD-2026-001 状态变为：待支付
[日志] order=ORD-2026-001, state=待支付
[第三方支付] trade=ORD-2026-001, cents=41400
[短信] 订单 ORD-2026-001 状态变为：已支付
[日志] order=ORD-2026-001, state=已支付
[短信] 订单 ORD-2026-001 状态变为：已发货
[日志] order=ORD-2026-001, state=已发货
[业务拒绝] 当前状态不能取消
```

### 练习与验收

1. 增加“满 500 减 80”策略，不能修改 `Order` 和 `ShopService`。
2. 增加余额校验，并将其插入库存校验之后；原校验器不能被修改。
3. 再接入一个返回 `true/false` 的现代支付 SDK，客户端仍只依赖 `PaymentGateway`。
4. 增加邮件观察者，并确保订单逻辑不包含任何邮件代码。
5. 调整程序，让未发货的已支付订单执行 `undo()` 时退款并取消；为失败的支付增加重试命令。
6. 思考：观察者若比订单先析构会怎样？尝试用 `weak_ptr` 改造生命周期管理。

---

## Demo 2：文件安全审计与报表系统

### 任务场景

企业需要扫描一棵虚拟文件目录。普通用户只能扫描公开目录，管理员可以扫描全部目录；安全规则由对象组合表达，并且扫描结果可以导出为不同格式。以后还会增加文件类型、审计指标、规则和报表格式。

### 功能要求

1. 文件和目录组成树形结构，客户端可以用统一接口处理两者。
2. 相同扩展名的文件共享不可变的类型元数据，避免重复对象。
3. 在不修改文件、目录类的情况下增加“统计”和“风险扫描”两种操作。
4. 风险规则表达式支持 `AND`、扩展名匹配和文件大小判断，并能自由组合。
5. 扫描服务代理必须在访问敏感目录前检查角色权限。
6. 报表生成流程固定为“标题 -> 正文 -> 页脚”，子类只定制具体格式。
7. 客户端通过安全中心外观调用扫描和导出功能。

### 模式分工

| 模式 | 在本例中的职责 | 主要变化点 |
|---|---|---|
| Composite | 表示文件与目录树 | 树的层级和节点数量 |
| Flyweight | 共享文件类型元数据 | 大量重复的内部状态 |
| Visitor | 对节点执行统计或审计 | 作用于稳定结构的新操作 |
| Interpreter | 组合安全规则表达式 | 规则语法与组合 |
| Proxy | 在扫描前执行访问控制 | 服务访问策略 |
| Template Method | 固定报表生成骨架 | 报表各步骤的实现 |
| Factory Method | 创建具体报表生成器 | 输出格式 |
| Facade | 提供审计与导出的统一入口 | 子系统协作流程 |

### UML 与流程分析

#### 核心类图

`Node` 层次负责稳定的数据结构，`NodeVisitor` 层次负责不断增加的操作；两者通过 `accept/visit` 实现双分派。规则表达式本身也是一棵小型组合树。

```mermaid
classDiagram
    direction LR

    class Node {
        <<interface>>
        -name: string
        +accept(visitor)
    }
    class File {
        -sizeKb: size_t
        -type: shared_ptr~const FileType~
        +accept(visitor)
    }
    class Directory {
        -sensitive: bool
        -children: vector~unique_ptr Node~
        +add(child)
        +accept(visitor)
    }
    Node <|-- File
    Node <|-- Directory
    Directory *-- Node : children

    class FileType {
        -extension: string
        -category: string
    }
    class FileTypeFactory {
        -cache: map
        +get(extension, category) FileType
    }
    File --> FileType : shares
    FileTypeFactory o-- FileType : caches

    class NodeVisitor {
        <<interface>>
        +visit(file)
        +visit(directory)
    }
    class StatisticsVisitor
    class RiskVisitor
    NodeVisitor <|.. StatisticsVisitor
    NodeVisitor <|.. RiskVisitor
    Node ..> NodeVisitor : accepts

    class RuleExpression {
        <<interface>>
        +interpret(file) bool
        +describe() string
    }
    class ExtensionIs
    class SizeGreaterThan
    class AndExpression
    RuleExpression <|.. ExtensionIs
    RuleExpression <|.. SizeGreaterThan
    RuleExpression <|.. AndExpression
    AndExpression *-- RuleExpression : left / right
    RiskVisitor --> RuleExpression : evaluates

    class ScanService {
        <<interface>>
        +scan(root, role, rule) vector~string~
    }
    class RealScanService
    class SecureScanProxy
    ScanService <|.. RealScanService
    ScanService <|.. SecureScanProxy
    SecureScanProxy *-- RealScanService : protects

    class ReportExporter {
        <<abstract>>
        +exportReport(risks) string
        #header() string
        #body(risks) string
        #footer() string
    }
    class TextReportExporter
    class JsonReportExporter
    ReportExporter <|-- TextReportExporter
    ReportExporter <|-- JsonReportExporter

    class ExporterCreator {
        <<interface>>
        +create() ReportExporter
    }
    class TextExporterCreator
    class JsonExporterCreator
    ExporterCreator <|.. TextExporterCreator
    ExporterCreator <|.. JsonExporterCreator
    ExporterCreator ..> ReportExporter : creates

    class SecurityCenter {
        +audit(root, role, rule, creator) string
    }
    SecurityCenter --> ScanService
    SecurityCenter --> ExporterCreator
```

#### 文件对象树

组合模式让叶节点与容器节点都实现 `Node`。访问者从根目录出发，按深度优先顺序访问图中的所有节点。

```mermaid
flowchart TD
    Root["Directory: server<br/>sensitive = true"]
    Src["Directory: src"]
    Logs["Directory: logs"]
    Main["File: main.cpp<br/>12 KB"]
    Worker["File: worker.cpp<br/>25 KB"]
    Access["File: access.log<br/>80 KB"]
    Leak["File: leak.log<br/>2048 KB"]

    Root --> Src
    Root --> Logs
    Src --> Main
    Src --> Worker
    Logs --> Access
    Logs --> Leak
```

#### 审计时序图

代理先完成权限检查；通过后，真实服务创建风险访问者遍历对象树。最后，外观通过工厂方法获得报表对象，并调用模板方法生成结果。

```mermaid
sequenceDiagram
    autonumber
    actor Client as 客户端
    participant Center as SecurityCenter
    participant Proxy as SecureScanProxy
    participant Scanner as RealScanService
    participant Visitor as RiskVisitor
    participant Tree as Directory / File Tree
    participant Rule as RuleExpression
    participant Creator as ExporterCreator
    participant Exporter as ReportExporter

    Client->>Center: audit(root, Admin, rule, creator)
    Center->>Proxy: scan(root, Admin, rule)
    Proxy->>Proxy: 检查敏感目录权限
    Proxy->>Scanner: scan(root, Admin, rule)
    Scanner->>Visitor: create(rule)
    Scanner->>Tree: accept(visitor)
    loop 深度优先遍历每个节点
        Tree->>Visitor: visit(node)
        opt node 是 File
            Visitor->>Rule: interpret(file)
            Rule-->>Visitor: 是否命中
        end
    end
    Visitor-->>Scanner: risks
    Scanner-->>Proxy: risks
    Proxy-->>Center: risks
    Center->>Creator: create()
    Creator-->>Center: ReportExporter
    Center->>Exporter: exportReport(risks)
    Note over Exporter: header -> body -> footer
    Exporter-->>Center: report text
    Center-->>Client: report text
```

### 可执行示例（C++14）

```cpp
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class File;
class Directory;

// ---------- Visitor ----------
class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;
    virtual void visit(const File&) = 0;
    virtual void visit(const Directory&) = 0;
};

// ---------- Flyweight ----------
class FileType {
public:
    FileType(std::string extension, std::string category)
        : extension_(std::move(extension)), category_(std::move(category)) {}

    const std::string& extension() const { return extension_; }
    const std::string& category() const { return category_; }

private:
    std::string extension_; // 内部状态：可共享且不可变
    std::string category_;
};

class FileTypeFactory {
public:
    std::shared_ptr<const FileType> get(const std::string& extension,
                                        const std::string& category) {
        auto it = cache_.find(extension);
        if (it != cache_.end()) return it->second;
        auto type = std::make_shared<const FileType>(extension, category);
        cache_[extension] = type;
        return type;
    }

    std::size_t typeCount() const { return cache_.size(); }

private:
    std::map<std::string, std::shared_ptr<const FileType>> cache_;
};

// ---------- Composite ----------
class Node {
public:
    explicit Node(std::string name) : name_(std::move(name)) {}
    virtual ~Node() = default;
    const std::string& name() const { return name_; }
    virtual void accept(NodeVisitor&) const = 0;

private:
    std::string name_; // 外部状态：每个节点不同
};

class File final : public Node {
public:
    File(std::string name, std::size_t sizeKb,
         std::shared_ptr<const FileType> type)
        : Node(std::move(name)), sizeKb_(sizeKb), type_(std::move(type)) {}

    std::size_t sizeKb() const { return sizeKb_; }
    const FileType& type() const { return *type_; }
    void accept(NodeVisitor& visitor) const override { visitor.visit(*this); }

private:
    std::size_t sizeKb_;
    std::shared_ptr<const FileType> type_;
};

class Directory final : public Node {
public:
    Directory(std::string name, bool sensitive = false)
        : Node(std::move(name)), sensitive_(sensitive) {}

    void add(std::unique_ptr<Node> child) { children_.push_back(std::move(child)); }
    bool sensitive() const { return sensitive_; }
    const std::vector<std::unique_ptr<Node>>& children() const { return children_; }

    void accept(NodeVisitor& visitor) const override {
        visitor.visit(*this);
        for (const auto& child : children_) child->accept(visitor);
    }

private:
    bool sensitive_;
    std::vector<std::unique_ptr<Node>> children_;
};

// ---------- Interpreter ----------
class RuleExpression {
public:
    virtual ~RuleExpression() = default;
    virtual bool interpret(const File&) const = 0;
    virtual std::string describe() const = 0;
};

class ExtensionIs final : public RuleExpression {
public:
    explicit ExtensionIs(std::string extension) : extension_(std::move(extension)) {}

    bool interpret(const File& file) const override {
        return file.type().extension() == extension_;
    }
    std::string describe() const override { return "扩展名为 " + extension_; }

private:
    std::string extension_;
};

class SizeGreaterThan final : public RuleExpression {
public:
    explicit SizeGreaterThan(std::size_t kb) : kb_(kb) {}

    bool interpret(const File& file) const override { return file.sizeKb() > kb_; }
    std::string describe() const override {
        return "大小超过 " + std::to_string(kb_) + "KB";
    }

private:
    std::size_t kb_;
};

class AndExpression final : public RuleExpression {
public:
    AndExpression(std::unique_ptr<RuleExpression> left,
                  std::unique_ptr<RuleExpression> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    bool interpret(const File& file) const override {
        return left_->interpret(file) && right_->interpret(file);
    }
    std::string describe() const override {
        return "(" + left_->describe() + " AND " + right_->describe() + ")";
    }

private:
    std::unique_ptr<RuleExpression> left_;
    std::unique_ptr<RuleExpression> right_;
};

class StatisticsVisitor final : public NodeVisitor {
public:
    void visit(const File& file) override {
        ++files_;
        totalKb_ += file.sizeKb();
    }
    void visit(const Directory&) override { ++directories_; }

    std::string result() const {
        std::ostringstream out;
        out << "目录数=" << directories_ << ", 文件数=" << files_
            << ", 总大小=" << totalKb_ << "KB";
        return out.str();
    }

private:
    std::size_t files_ = 0;
    std::size_t directories_ = 0;
    std::size_t totalKb_ = 0;
};

class RiskVisitor final : public NodeVisitor {
public:
    explicit RiskVisitor(const RuleExpression& rule) : rule_(rule) {}

    void visit(const File& file) override {
        if (rule_.interpret(file)) risks_.push_back(file.name());
    }
    void visit(const Directory&) override {}

    const std::vector<std::string>& risks() const { return risks_; }

private:
    const RuleExpression& rule_;
    std::vector<std::string> risks_;
};

// ---------- Proxy ----------
enum class Role { Employee, Admin };

class ScanService {
public:
    virtual ~ScanService() = default;
    virtual std::vector<std::string> scan(const Directory&, Role,
                                          const RuleExpression&) const = 0;
};

class RealScanService final : public ScanService {
public:
    std::vector<std::string> scan(const Directory& root, Role,
                                  const RuleExpression& rule) const override {
        RiskVisitor visitor(rule);
        root.accept(visitor);
        return visitor.risks();
    }
};

class SecureScanProxy final : public ScanService {
public:
    explicit SecureScanProxy(std::unique_ptr<ScanService> target)
        : target_(std::move(target)) {}

    std::vector<std::string> scan(const Directory& root, Role role,
                                  const RuleExpression& rule) const override {
        if (root.sensitive() && role != Role::Admin) {
            throw std::runtime_error("权限不足：敏感目录仅管理员可扫描");
        }
        std::cout << "[审计日志] 开始扫描目录：" << root.name() << '\n';
        return target_->scan(root, role, rule);
    }

private:
    std::unique_ptr<ScanService> target_;
};

// ---------- Template Method + Factory Method ----------
class ReportExporter {
public:
    virtual ~ReportExporter() = default;

    std::string exportReport(const std::vector<std::string>& risks) const {
        return header() + body(risks) + footer(); // 固定算法骨架
    }

private:
    virtual std::string header() const = 0;
    virtual std::string body(const std::vector<std::string>&) const = 0;
    virtual std::string footer() const = 0;
};

class TextReportExporter final : public ReportExporter {
    std::string header() const override { return "=== 安全审计报告 ===\n"; }
    std::string body(const std::vector<std::string>& risks) const override {
        std::ostringstream out;
        for (const auto& name : risks) out << "- 风险文件：" << name << '\n';
        if (risks.empty()) out << "- 未发现风险\n";
        return out.str();
    }
    std::string footer() const override { return "=== 报告结束 ===\n"; }
};

class JsonReportExporter final : public ReportExporter {
    std::string header() const override { return "{\"risks\":["; }
    std::string body(const std::vector<std::string>& risks) const override {
        std::ostringstream out;
        for (std::size_t i = 0; i < risks.size(); ++i) {
            if (i != 0) out << ',';
            out << '\"' << risks[i] << '\"'; // 示例文件名不包含需转义字符
        }
        return out.str();
    }
    std::string footer() const override { return "]}\n"; }
};

class ExporterCreator {
public:
    virtual ~ExporterCreator() = default;
    virtual std::unique_ptr<ReportExporter> create() const = 0;
};

class TextExporterCreator final : public ExporterCreator {
public:
    std::unique_ptr<ReportExporter> create() const override {
        return std::unique_ptr<ReportExporter>(new TextReportExporter);
    }
};

class JsonExporterCreator final : public ExporterCreator {
public:
    std::unique_ptr<ReportExporter> create() const override {
        return std::unique_ptr<ReportExporter>(new JsonReportExporter);
    }
};

// ---------- Facade ----------
class SecurityCenter {
public:
    explicit SecurityCenter(const ScanService& scanner) : scanner_(scanner) {}

    std::string audit(const Directory& root, Role role,
                      const RuleExpression& rule,
                      const ExporterCreator& creator) const {
        auto risks = scanner_.scan(root, role, rule);
        auto exporter = creator.create();
        return exporter->exportReport(risks);
    }

private:
    const ScanService& scanner_;
};

int main() {
    try {
        FileTypeFactory types;
        auto cpp = types.get(".cpp", "source");
        auto log = types.get(".log", "log");

        Directory root("server", true);
        auto source = std::unique_ptr<Directory>(new Directory("src"));
        source->add(std::unique_ptr<Node>(new File("main.cpp", 12, cpp)));
        source->add(std::unique_ptr<Node>(new File("worker.cpp", 25, cpp)));

        auto logs = std::unique_ptr<Directory>(new Directory("logs"));
        logs->add(std::unique_ptr<Node>(new File("access.log", 80, log)));
        logs->add(std::unique_ptr<Node>(new File("leak.log", 2048, log)));
        root.add(std::move(source));
        root.add(std::move(logs));

        StatisticsVisitor statistics;
        root.accept(statistics);
        std::cout << "[统计] " << statistics.result()
                  << "，共享类型对象数=" << types.typeCount() << '\n';

        AndExpression riskyLog(
            std::unique_ptr<RuleExpression>(new ExtensionIs(".log")),
            std::unique_ptr<RuleExpression>(new SizeGreaterThan(1024)));
        std::cout << "[规则] " << riskyLog.describe() << '\n';

        SecureScanProxy proxy(std::unique_ptr<ScanService>(new RealScanService));
        SecurityCenter center(proxy);
        TextExporterCreator textCreator;
        JsonExporterCreator jsonCreator;

        try {
            center.audit(root, Role::Employee, riskyLog, textCreator);
        } catch (const std::exception& e) {
            std::cout << "[拒绝] " << e.what() << '\n';
        }

        std::cout << center.audit(root, Role::Admin, riskyLog, textCreator);
        std::cout << center.audit(root, Role::Admin, riskyLog, jsonCreator);
    } catch (const std::exception& e) {
        std::cerr << "审计失败：" << e.what() << '\n';
        return 1;
    }
}
```

编译运行：

```bash
g++ -std=c++14 -Wall -Wextra -pedantic demo_audit.cpp -o demo_audit
./demo_audit
```

预期输出：

```text
[统计] 目录数=3, 文件数=4, 总大小=2165KB，共享类型对象数=2
[规则] (扩展名为 .log AND 大小超过 1024KB)
[拒绝] 权限不足：敏感目录仅管理员可扫描
[审计日志] 开始扫描目录：server
=== 安全审计报告 ===
- 风险文件：leak.log
=== 报告结束 ===
[审计日志] 开始扫描目录：server
{"risks":["leak.log"]}
```

### 练习与验收

1. 实现 `OrExpression` 和 `NotExpression`，组合出“超过 1MB 的日志文件，或任意 `.exe` 文件”。
2. 新增 `ChecksumVisitor`，不能修改 `File`、`Directory` 和已有访问者。
3. 新增 CSV 报表，只增加新的导出器和创建器，不修改 `SecurityCenter`。
4. 为代理增加扫描结果缓存；思考缓存键应包含目录版本、角色和规则中的哪些信息。
5. 当前代理只检查根目录。改造设计，使普通用户扫描公开根目录时也不能越权进入其中的敏感子目录。
6. 当前 JSON 示例没有处理转义。补全引号、反斜杠和控制字符的转义，并为报表添加断言测试。

---

## 综合复盘

完成练习后，不要只回答“用了哪些模式”，还应能回答以下问题：

1. **对象的职责是否单一？** 例如订单不负责调用具体支付 SDK，目录节点不负责生成具体报表。
2. **高层是否依赖抽象？** `ShopService` 依赖 `PaymentGateway`，`SecurityCenter` 依赖 `ScanService` 和 `ExporterCreator`。
3. **新增需求主要通过扩展还是修改完成？** 新增策略、访问者、导出器时，应尽量新增类而不是改稳定代码。
4. **继承是否真的表达“可替换关系”？** 仅为复用代码而继承通常会破坏里氏替换原则，优先考虑组合。
5. **模式带来的复杂度是否值得？** 若系统永远只有一种价格、一种输出格式，就不必预先建立庞大的抽象层。
6. **资源所有权是否清晰？** 示例用 `unique_ptr` 表示独占所有权，用 `shared_ptr<const FileType>` 表示共享的不可变享元；非拥有型指针必须保证被引用对象活得足够久。

真正的综合练习目标，是能从需求变化中找到稳定部分与变化部分，并让对象通过清晰、最小的接口协作，而不是机械地堆叠 23 个模式。
