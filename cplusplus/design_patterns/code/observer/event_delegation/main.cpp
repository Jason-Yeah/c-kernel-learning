#include "listview.hpp"

int main()
{
    ListView list;
    list.add_item("A001", "张三")
        .add_item("A002", "李四")
        .add_item("A003", "王五")
        .add_item("A004", "赵六");

    // ★★★ 只有一个回调！不需要 4 个，不需要 100 个 ★★★
    list.on_item_click(
        [](int index, const ListItem &item)
        {
            std::cout << "  → 处理: " << item.get_id() << " (" << item.data()
                      << ") @ index " << index << std::endl;
        });

    // 模拟点击
    list.SimulateClick(0);
    list.SimulateClick(2);
    list.SimulateClick(3);

    return 0;
}