# Iterator

C++中增强版指针，例如

```cpp
std::vector<int> vec;
```

## const迭代器

```cpp
const std::vector<int>::iterator iter = vec.begin();
```

本质：T* const指针

- 锁住了iter本身，iter此时就焊死在vec.begin()上，所以执行`iter ++ ;`是错误的，因为iter就在vec.begin()上，死都不能动
- 没锁住内容，这里是顶层const只锁变量（这里是迭代器）本身，内容没锁，所以你`*iter = 10;`没问题。

实际上锁的是位置，而不是内容，为啥是位置，因为这里iter是指针，所以这样理解。相当于人锁在原地不能动，但可以随便动手够者你能够到的东。西

## const_iterator锁内容

```cpp
std::vector<int>::const_iterator cIter = vec.begin();
```

本质：const T*

- 没锁住cIter本事，这里相当于底层指针，锁的是内容，内容是常量只读不可写，本身不锁，所以`cIter ++ ;`没问题，可以指向下一个元素。
- 锁住了内容，只读不写，所以`*cIter = 10;`是禁止的。

相当于可以在屋子里乱走，但屋子里东西不能动。
