# Document
## Class RotateStackedWidget
* 带有翻转动画的 Stacked Widget
* Example
```cpp
RotateStackedWidget *stackedWidget = new RotateStackedWidget(this);
stackedWidget->addWidget(widget_1);
stackedWidget->addWidget(widget_2);
stackedWidget->setCurrentIndex(0);
stackedWidget->rotate(1);           // 页面翻转
```
-----
## Class Toast
* Toast 提示窗

    注意：
    * 1.当 `parent == nullptrt` 时，Toast会出现在活动桌面水平居中垂直 3/4 的地方，`parent != nullptrt` 时则Toast会出现在父窗口水平居中垂直 3/4 的地方。
    * 2.当 Toast 正在显示的时候，重新调用时 `Toast::toast()` 将重新显示消息。
* Example
```cpp
Toast *toast = new Toast(this);
toast->toast("Hello");
```
-----
## Class TranslationStackedWidget
* 具有平移动画的 Stacked Widget
* Example
```cpp
TranslationStackedWidget *stackedWidget = new TranslationStackedWidget(this);
stackedWidget->addWidget(widget_1);
stackedWidget->addWidget(widget_2);
stackedWidget->setCurrentIndex(0);
stackedWidget->moveToIndex(1);           // 页面平移
```