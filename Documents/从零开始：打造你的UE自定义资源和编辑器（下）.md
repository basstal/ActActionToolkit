14. 承接上文，讲完 Slate 的槽点以后，简单介绍一下 Slate 的一些基础知识：

![012](Images/012.png)

| 名称               | 作用                                                                                                                                                                                                                                             | 派生类型                                                                      |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------- |
| SWidget            | 所有 Slate Widget 的基类，UE 不建议直接派生自这个类型，建议如果没有子节点可以继承自 SLeafWidget，有子节点则继承自 SPanel                                                                                                                         | SCompoundWidget 、 SPanel 、 SLeafWidget 、 SNullWidgetContent 、 SWeakWidget |
| SCompoundWidget    | 大部分只有一个子节点的 Widget 都派生自这个类型，比较常用的有 SBorder(带有 BorderImage 属性的单子节点 Widget) 、 SDetailView(UObject反射属性查看使用的 Widget) 、 SMessageLog(消息提示)                                                           | SBorder 、 SDetailView 、 SMessageLog 、 SSequencerTrackLane 等               |
| SPanel             | SPanel 中有个关键函数 OnArrangeChildren 其参数 AllottedGeometry (代表 SPanel 的 Geometry 信息) 、ArrangedChildren (SPanel 的 Child Widget 都需要通过 AddWidget 添加到这个结构中)， SPanel 的子类通过重载这个函数，使其具有不同的分配子节点方式。 | SListPanel 、 SBox 、 SGridPanel 、 SScrollPanel 、 SOverlay                  |
| SLeafWidget        | 该 Widget 不能添加子节点，比较简单只做最终一个区域的绘制用                                                                                                                                                                                       | SSlider 、 SSpacer 、 SImage 、 SAnimNotifyNode                               |
| SNullWidgetContent | 没有任何大小的空Widget，也不能够添加子节点，一般使用 SNullWidget::NullWidget 获取，并作为参数给需要 SWidget 的函数占位用                                                                                                                         |                                                                               |
| SWeakWidget        | 一般来说父节点对子节点的引用都是强引用，保证子节点不会被GC，若子节点是 SWeakWidget ，则子节点生命周期不受父节点影响，官方的例子是 Hover 一个 Widget 显示 tooltip 时，弹出的 Window 对于这个被 Hover 的 Widget 而言必须是 SWeakWidget             |                                                                               |

15. 在掌握了 Slate 定义了哪些 Widget 后，可以开始进一步了解如何写自定义的 Slate Widget，首先挑选一个上文提到的类型作为基类，当然如果长相一致也可以考虑继承那些由上文类型派生的类型，例如想快速建立一个树状结构的列表，则直接继承自 STreeView，下面就拿这个来举例：

```C++
class SActImageTreeView : public STreeView<TSharedRef<SActImageTreeViewTableRow>>
```

STreeView 模板要求 ItemType 是一个指针类型，这里统一使用 SharedRef 的方式，来确保这个指针的内容不会为空。接下来是具体类型，这里例子中选择的是派生自 SMultiColumnTableRow ，也可以派生自其他更简单的 SWidget ，例如每个 Item 只需要一列的话可以直接派生自 STableRow 等。因为这里的界面设计上需要兼容一个 TreeView 的 Item 就将多列的内容包揽在内， 所以用 SMultiColumnTableRow ， 通过重载 GenerateWidgetForColumn 能够根据参数 InColumnName 来区分每列需要创建的内容 Widget。注意到模板也是需要传入 SharedRef ，不过这次指针对象类型可以是自身，这样表示对应的 Model 数据也来自 Widget。

```C++
class SActImageTreeViewTableRow : public SMultiColumnTableRow<TSharedRef<SActImageTreeViewTableRow>>
```

这里就顺带说一下，看到很多 Widget 代码中都记录了  ``SharedPtr<XXXModel>`` 有些甚至还有辅助的 class 来记录或转发控制信息，起初以为是 MVC 的设计，但是后来仔细看了一些以后发现也不是这样，大部分 Model 出现的地方都是做 Widget 的接口，实际上 Widget 所控制的 Model 会是这个 XXXModel 的派生类，然后派生类再去实现一些具体的设计，这样可以把一些互相之间无关的类型放到一个 Widget 中做显示和数据传递。例如 FAnimTimelineTrack 、 FAnimModel 这样，实际上它们的派生类会有很多不同的数据需要处理，不同的显示绘制方式，但是 Widget 可以只使用 FAnimModel 就完成所有的事情。 UE 做了很多**接口和实现分离**的事情。有些看起来是必须的，但是有些却显得有点多余，不过也可能是为了将来的扩展考虑吧。但是大面积的这样做也存在一个问题，大部分都是在设计和考虑不够充分的情况下，会看到函数的参数列表分给不同的实现去使用的情况，例如参数1给A实现用，B实现只用参数2，C实现又用到参数1和3。导致一些无谓的传参和函数作用的不解，在 Delegate 中这种情况就更是屡见不鲜了，因为之前说过的 Widget 将自己的参数不断往下层 Widget 传递，这些定义好的 Delegate 函数也在往下传，如果中间有任何 Widget 需要往 Delegate 中添加参数使之往上层传，都要加到 Delegate 定义的参数列表中，这样传了几次后，每个 Widget 都可能加了几个不同的参数，就导致了参数多而无用的情况出现。

16. 刚才一段题外话，还是回到 Slate Widget 的写法来，在继承了理想的基类后，需要明确定义一个 FArguments ，这个结构目前有两种方式来定义，常见的是使用宏来定义，通过查看 DeclarativeSyntaxSupport.h 文件中宏定义可以看到，宏的作用是定义一个 class 内部的 struct FArguments ，同时给这个参数列表加了很多默认的字段上去，这个默认字段是为了传参给 SWidget 基类使用。

```C++
...
SLATE_BEGIN_ARGS(SActImageTreeView) { }
    ...
SLATE_END_ARGS() 
// #define SLATE_END_ARGS() \
// };
...
```

![013](Images/013.png)

那么顺水推舟，另一种方式就是直接将宏的内容给复制出来，并不是/dodge，另一种方式之前也提到过，就是直接顺手牵羊，巧夺名义。typedef 其他 Widget 的参数作为自己的参数，其实这样如果刚好也是继承自那个 Widget 的话，就基本一模一样了。那么想要不一样的话，就得从构造开始一步步改变自身， Slate Widget 很少用到构造函数， 所谓的 Slate 构造都是名为 Construct 的函数开始。这个函数也必须将第一个参数定义为 const FArguments&，原因就在 Slate 的构造宏 SNew 和 SAssignNew 中，这些宏改造了第一个参数的传入过程(也是前文提到奇怪的连续赋值的始作俑者)，使得可以直接在构造宏后边接 Slate 声明在参数列表中的参数，修改这些参数的值，还可以直接操作这个 Widget 的 Slot 添加子节点，因为 Slot 其实也是这个参数列表的参数之一。

```C++
...
public:
    typedef STreeView::FArguments FArguments;

    void Construct(const FArguments& InArgs, const TSharedRef<SActImagePoolAreaPanel>& InActImageTrackAreaPanel);
...
```
```C++
// SNew 简单写法示例
SNew(SBorder)
.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))   // SBorder -> SLATE_ATTRIBUTE( const FSlateBrush*, BorderImage )
.Padding(FMargin(0.6f, 0.3f))                                   // SBorder -> SLATE_ATTRIBUTE( FMargin, Padding )
.Clipping(EWidgetClipping::ClipToBounds)                        // FArguments 基类 TSlateBaseNamedArgs -> SLATE_PRIVATE_ARGUMENT_FUNCTION(EWidgetClipping, Clipping)
[   
    SNew(SHorizontalBox) // 单个子节点的情况，直接使用中括号添加内容
]
```
```C++
// SAssignNew 简单写法示例
TSharedPtr<SGridPanel> GridPanel = nullptr;
SAssignNew(GridPanel, SGridPanel)
.FillRow(1, 1.0f)                   // SGridPanel -> FArguments& FillRow( int32 RowId, const TAttribute<float>& Coefficient )
.FillColumn(0, ...)                 // SGridPanel -> FArguments& FillColumn( int32 ColumnId, const TAttribute<float>& Coefficient )
.FillColumn(1, ...)                 // SGridPanel -> FArguments& FillColumn( int32 ColumnId, const TAttribute<float>& Coefficient )
+ SGridPanel::Slot(0, 0)            // 多个子节点的情况，先通过 + XXX::Slot() 添加子节点 Slot ，后中括号接子节点内容
[
    SNew(SBorder)
]
+ SGridPanel::Slot(...)
[
    ...
]
```

上面列举了 SNew 和 SAssignNew 的基本用法，以及一些奇怪的 Slate 语法规则，在 UE 文档中看到 Slate 的作者们信誓旦旦的说``Slate's composition framework makes it simple to re-arrange UI elements quickly for prototyping and iteration.``，感觉应该是跟纯 C++ 代码去撸来比较的。实际上拆开这些宏来看，这跟纯 C++ 也没啥大区别，就是定义了些代码格式化工具无法识别的符号，代替了手写一大堆要靠自动补全才敲得出来的代码。总结一句话，也没好到哪里去。借助之前知乎看的原话：``UE4的UI解决方案一直都是Slate表示的，只是上层封了层UMG之后感觉UE4的团队就很少有精力扩展Slate的东西了。许多UI层的组件需求和效果需求反应都很迟钝。``而且关键的关键！这个方式做游戏UI是要做死的，每改一个布局一个属性都需要重新编译一次代码，就算有HotReload，迭代起来也比那些脚本语言支持的UI框架不知道要慢到哪去了。或许这也是为啥 Slate 没有后续扩展的原因。

17.  Construct 函数通过构造宏来调用，除了第一个参数必须满足既定规则以外，其他参数倒是可以随便传，终归来也没太搞懂用 FArguments 传参的好处在哪，或许是写法上的统一和解决构造时需要修改基类参数的问题吧。不论如何，写在 Construct 的参数列表后面，和预先定义在 Widget SLATE_BEGIN_ARGS ... SLATE_END_ARGS 之间，效果是差不多的。这两种方式都没有把你定义的参数替你保存在构造的 Widget 中，还是需要你在 Widget 中再定义一个对应的字段去保存这个参数(当然有些参数只在构造时使用就另说了/dodge)。这里只顺带提一下如何在 SLATE_BEGIN_ARGS ... SLATE_END_ARGS 之间定义参数，因为非强制的情况下个人不用这种方式。这些宏也都在 DeclarativeSyntaxSupport.h 文件中定义。

```C++
SLATE_BEGIN_ARGS(SBorder) { }
    ...
    SLATE_ARGUMENT(EHorizontalAlignment, HAlign)                // 声明字段参数，一般就是 Enum struct class 基本数据类型 这些
    ...
    SLATE_ATTRIBUTE(FMargin, Padding)                           // 比较特殊的字段参数，会用 TAttribute 包装给的类型
    ...
    SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)        // 声明 Delegate 参数
    ...
SLATE_END_ARGS()
```

18.  有了构造和参数，剩下的只有怎么绘制的问题了。绘制的代码基本也是抄一抄，要不然没有设计的情况下也不知道这个 Widget 要长什么样比较合适(或者也可以拍脑袋自己设计/dodge)。说到学(抄)代码，不得不介绍一个利器，这是个人认为 UE 的 Slate 做得最好的地方，就是下图所示的工具 Widget Reflector。

![014](Images/014.png)

用法也很简单，一种是使用 Pick 功能正向查找想要学习的 Widget，另一种是反向查找，点击树状结构中的节点，通过查看对应框中(绿色/白色)区域来确定是不是要的那个区域。这里只演示正向查找。

![015](Images/015.gif)

当确定当前区域是想要学习的区域后，并且编辑器也正确的链接到 IDE 后，就可以按如下方式快速定位 Widget 的代码了。

![016](Images/016.gif)

19.  恭喜你！已经大致入门了自定义 UE 资源和 Slate 界面，接下来就是 UMG 的学习或者深入到 SlateCore 中看代码。说起 UMG ， puerts 的作者提出用 react 绑定 UMG 对象的想法还挺不错的，大致上就是让 UMG UI 套用成熟的 react 写法，实际核心还是需要靠映射的 UMG UI 组件来完成，感觉至少比坑爹的写 C++ Slate 要好。缺点就是如果美术要进这套工作流，缺少图形化界面来辅助，不可能让美术去写 react 代码。

## 参考文献

- [1] [Slate UI Framework](https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/Slate/)
- [2] [UE4中UI解决方案Slate](https://zhuanlan.zhihu.com/p/24726208)
- [3] [UE4下玩转react](https://zhuanlan.zhihu.com/p/240617365)