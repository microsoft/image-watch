using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace Microsoft.ImageWatch.Interface
{
    /// <summary>
    /// Interaction logic for WatchList.xaml
    /// </summary>
    public partial class WatchList : UserControl
    {
        public WatchList()
        {
            InitializeComponent();
            InitializeData();

            PreviewKeyDown += WatchList_PreviewKeyDown;
            listBox.SelectionChanged += listBox_SelectionChanged;
            listBox.SizeChanged += listBox_SizeChanged;
        }

        private void ScrollCurrentItemIntoView()
        {
            if (SelectedItem != null)
                listBox.ScrollIntoView(SelectedItem);
        }

        void listBox_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            ScrollCurrentItemIntoView();
        }

        void listBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ScrollCurrentItemIntoView();

            if (SelectedItemChanged != null)
                SelectedItemChanged(this, EventArgs.Empty);

            if (e.RemovedItems != null)
            {
                lastSelectedItem_ = e.RemovedItems.Count > 0 ? (e.RemovedItems[0] as WatchListItem) : null;

                if (lastSelectedItem_ == nullItem_)
                    lastSelectedItem_ = null;
            }
        }

        WatchListItem lastSelectedItem_ = null;
        public void SelectLastSelectedItem()
        {
            if (lastSelectedItem_ != null)
                SelectedItem = lastSelectedItem_;
        }

        private bool userCanModify_ = false;
        public bool UserCanModify
        {
            get
            {
                return userCanModify_;
            }
            set
            {
                userCanModify_ = value;
                InitializeData();
            }
        }

        private WatchListItem nullItem_ = new WatchListItem(String.Empty);
        private CompositeCollection data_ = new CompositeCollection();

        void InitializeData()
        {
            data_.Clear();
            data_.Add(itemContainer_);

            if (UserCanModify)
            {
                var nullContainer = new CollectionContainer();
                var nullCollection = new ObservableCollection<WatchListItem>();
                nullContainer.Collection = nullCollection;
                nullCollection.Add(nullItem_);

                data_.Add(nullContainer);
            }

            listBox.ItemsSource = data_;
        }

        private CollectionContainer itemContainer_ = new CollectionContainer();
        public ObservableCollection<WatchListItem> ItemsSource
        {
            get
            {
                return (ObservableCollection<WatchListItem>)
                    itemContainer_.Collection;
            }

            set
            {
                itemContainer_.Collection = value;
            }
        }

        public int SelectedIndex
        {
            get
            {
                return listBox.SelectedIndex;
            }
            set
            {
                if (listBox.Items.Count < 1)
                    return;

                listBox.SelectedIndex = Math.Min(listBox.Items.Count - 1,
                    Math.Max(0, value));
            }
        }

        public WatchListItem SelectedItem
        {
            get
            {
                return listBox.SelectedItem as WatchListItem;
            }
            set
            {
                if (listBox.Items == null)
                    return;

                if (value != null && !listBox.Items.Contains(value))
                    return;

                listBox.SelectedItem = value;
            }
        }

        public event EventHandler SelectedItemChanged;

        public WatchListItem NonNullSelectedItem
        {
            get
            {
                return SelectedItem == nullItem_ ? null : SelectedItem;
            }
        }

        WatchListItem GetItemAt(int index)
        {
            if (index < 0 || index > listBox.Items.Count - 1)
                return null;

            return listBox.Items[index] as WatchListItem;
        }

        WatchListItem GetItemFromSender(object sender)
        {
            var fe = sender as FrameworkElement;
            if (fe == null)
                return null;

            return fe.DataContext as WatchListItem;
        }

        ListBoxItem GetListBoxItemFromItem(WatchListItem item)
        {
            if (item == null)
                return null;

            return listBox.ItemContainerGenerator.ContainerFromItem(item)
                as ListBoxItem;
        }

        void BeginEdit(int index)
        {
            var item = GetItemAt(index);
            if (item == null)
                return;

            item.IsEditing = true;

            var listBoxItem = GetListBoxItemFromItem(item);
            if (listBoxItem == null)
                return;

            var tb = WPFHelpers.FindVisualChild<TextBox>(listBoxItem);
            if (tb != null)
                tb.Focus();
        }

        void CommitEdit(int index, bool lostFocus)
        {
            var item = GetItemAt(index);
            if (item == null)
                return;

            item.IsEditing = false;

            var listBoxItem = GetListBoxItemFromItem(item);
            if (listBoxItem == null)
                return;

            var tb = WPFHelpers.FindVisualChild<TextBox>(listBoxItem);
            var text = tb != null ? tb.Text.Trim() : null;

            if (item == nullItem_)
            {
                if (!string.IsNullOrWhiteSpace(text))
                {
                    // this is tricky: adding a new item where the selected
                    // item was LOSES focus. Then when selection is changed, 
                    // listbox somehow brings the focus back and overwrites
                    // your selection with the empty last element ...
                    // the current solution works OK: it uses UpdateLayout 
                    // to bring up all the listboxitems, then selects the 
                    // new one. ONLY if the reason for commmit was not a 
                    // focus loss (i.e. clicking somewhere else as opposed to
                    // hitting "Return"), we set the focus on the new element
                    var controller = DataContext as Controller;
                    if (controller != null && SelectedItem != null)
                        controller.AddToWatchList(text);

                    Dispatcher.BeginInvoke(new Action(() =>
                        {
                            UpdateLayout();
                            SelectedIndex = index;
                            if (!lostFocus)
                                FocusSelectedItem();
                        }));
                }
            }
            else
            {
                if (!string.IsNullOrWhiteSpace(text))
                {
                    if (item.Expression != text)
                        item.Expression = text;
                    item.NotifyAllPropertiesChanged();
                }
                else
                {
                    DeleteItem(index, lostFocus);
                }
            }
        }

        private void FocusSelectedItem()
        {
            var lbi = GetListBoxItemFromItem(SelectedItem);

            if (lbi != null)
                lbi.Focus();
        }

        void CancelEdit(int index)
        {
            var item = GetItemAt(index);
            if (item == null)
                return;

            item.IsEditing = false;
        }

        void DeleteItem(int index, bool lostFocus)
        {
            if (listBox.Items != null &&
                index < listBox.Items.Count)
            {
                var item = GetItemAt(index);

                if (ItemsSource.Contains(item))
                {
                    ItemsSource.Remove(item);
                    item.Dispose();
                    SelectedIndex = index;
                    // deletion loses focus, so get it back (unless we lost
                    // focus by clicking away when the edit box was empty)
                    if (!lostFocus)
                        FocusSelectedItem();
                }
            }
        }

        public void MainControl_MouseLeftButtonDown(object sender,
            MouseButtonEventArgs e)
        {
            CommitSelectedEditIfEditing();
        }

        public void MainControl_PreviewMouseRightButtonDown(object sender,
            MouseButtonEventArgs e)
        {
            if (SelectedItem == null)
                return;

            var tb = WPFHelpers.FindVisualChild<TextBox>(
                GetListBoxItemFromItem(SelectedItem));

            if (tb == null)
                return;

            if (VisualTreeHelper.HitTest(tb, Mouse.GetPosition(tb)) == null)
                CommitSelectedEditIfEditing();
        }

        private void CommitSelectedEditIfEditing()
        {
            if (SelectedItem != null)
            {
                if (SelectedItem.IsEditing && listBox.Items != null
                    && listBox.Items.Contains(SelectedItem))
                    CommitEdit(listBox.Items.IndexOf(SelectedItem), false);
            }
        }

        void listBoxItem_RightMouseButtonDown(object sender, MouseEventArgs e)
        {
            // this disables selection on right-click. Not sure this is good.
            // It feels weird if context menu has item-specific commands, e.g.
            // "Add to Watch" ..
            //contextMenu.IsOpen = true;
            //e.Handled = true;
        }

        void listBoxItem_ExpandButtonLeftMouseDown(object sender,
            MouseEventArgs e)
        {
            var item = GetItemFromSender(sender);
            if (item == null)
                return;

            if (item != nullItem_)
                item.IsExpanded = !item.IsExpanded;

            e.Handled = true;
        }

        // unselect if clicking on non-item area of listbox
        private void listBox_MouseLeftButtonDown(object sender,
            MouseButtonEventArgs e)
        {
            CommitSelectedEditIfEditing();

            listBox.UnselectAll();
            e.Handled = true;
        }

        void expression_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (!UserCanModify)
                return;

            BeginEdit(SelectedIndex);
            e.Handled = true;
        }

        private void editBox_GotFocus(object sender, RoutedEventArgs e)
        {
            var tb = sender as TextBox;
            if (tb == null)
                return;

            var item = GetItemFromSender(sender);
            if (item == null)
                return;

            tb.Text = item.Expression;
            tb.SelectAll();
        }

        private void editBox_LostFocus(object sender, RoutedEventArgs e)
        {
            var tb = sender as TextBox;
            if (tb == null)
                return;

            var item = GetItemFromSender(sender);
            if (item == null)
                return;

            if (item.IsEditing && listBox.Items != null
                && listBox.Items.Contains(item))
                CommitEdit(listBox.Items.IndexOf(item), true);
        }

        private void editBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            e.Handled = false;

            switch (e.Key)
            {
                case Key.Return:
                    CommitEdit(SelectedIndex, false);
                    e.Handled = true;
                    break;
                case Key.Escape:
                    CancelEdit(SelectedIndex);
                    e.Handled = true;
                    break;
            }
        }

        void WatchList_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            e.Handled = false;

            var item = GetItemAt(SelectedIndex);

            if (item != null && item.IsEditing)
            {
                // let editbox handle keyboard events
            }
            else
            {
                if (UserCanModify &&
                   ((WPFHelpers.CtrlPressed() && e.Key == Key.X) ||
                   (WPFHelpers.CtrlPressed() && e.Key == Key.Delete)))
                {
                    if (item != null && item != nullItem_)
                    {
                        Clipboard.Clear();
                        Clipboard.SetText(item.Expression);
                    }
                    DeleteItem(SelectedIndex, false);
                    e.Handled = true;
                    return;
                }

                if (UserCanModify &&
                    ((WPFHelpers.CtrlPressed() && e.Key == Key.C) ||
                    (WPFHelpers.CtrlPressed() && e.Key == Key.Insert)))
                {
                    if (item != null && item != nullItem_)
                    {
                        Clipboard.Clear();
                        Clipboard.SetText(item.Expression);
                    }
                    e.Handled = true;
                    return;
                }

                if (UserCanModify &&
                    ((WPFHelpers.CtrlPressed() && e.Key == Key.V) ||
                    (WPFHelpers.ShiftPressed() && e.Key == Key.Insert)))
                {
                    BeginEdit(SelectedIndex);
                    e.Handled = false;
                    return;
                }

                if (UserCanModify && !WPFHelpers.CtrlPressed() &&
                    e.Key >= Key.D0 && e.Key <= Key.Z)
                {
                    BeginEdit(SelectedIndex);
                    e.Handled = false;
                    return;
                }

                // handle other not-editing keyboard input
                e.Handled = true;

                switch (e.Key)
                {
                    default:
                        e.Handled = false;
                        break;
                    case Key.Delete:
                        if (UserCanModify)
                            DeleteItem(SelectedIndex, false);
                        break;
                    case Key.Home:
                    case Key.PageUp:
                        SelectedIndex = 0;
                        break;
                    case Key.End:
                    case Key.PageDown:
                        SelectedIndex = int.MaxValue;
                        break;
                    case Key.Tab:
                        SelectedIndex =
                            SelectedIndex + (WPFHelpers.ShiftPressed() ? -1 : 1);
                        break;
                    case Key.Up:
                        --SelectedIndex;
                        break;
                    case Key.Down:
                        ++SelectedIndex;
                        break;
                    case Key.Space:
                    case Key.Back:
                    case Key.F2:
                        if (UserCanModify)
                            BeginEdit(SelectedIndex);
                        break;
                    case Key.Right:
                        if (item != null && item != nullItem_)
                            item.IsExpanded = true;
                        break;
                    case Key.Left:
                        if (item != null && item != nullItem_)
                            item.IsExpanded = false;
                        break;
                    case Key.Return:
                        if (item != null && item != nullItem_)
                            item.IsExpanded = !item.IsExpanded;
                        break;
                }
            }
        }

        void TooltipTextBlock_ToolTipOpening(object sender,
            ToolTipEventArgs e)
        {
            e.Handled = true;

            var tb = sender as TextBlock;
            if (tb != null && !string.IsNullOrWhiteSpace(tb.Text))
            {
                tb.Measure(new Size(Double.PositiveInfinity,
                     Double.PositiveInfinity));
                if (tb.ActualWidth < tb.DesiredSize.Width)
                {
                    tb.ToolTip = tb.Text;
                    e.Handled = false;
                }
            }
        }

        ////////
        // Context Menu

        private void ContextMenu_ExpandAllClick(object sender, RoutedEventArgs e)
        {
            foreach (WatchListItem item in ItemsSource)
                item.IsExpanded = true;

            ScrollCurrentItemIntoView();
        }

        private void ContextMenu_CollapseAllClick(object sender, RoutedEventArgs e)
        {
            foreach (WatchListItem item in ItemsSource)
                item.IsExpanded = false;

            ScrollCurrentItemIntoView();
        }

        private void ContextMenu_AddToWatchClick(object sender, RoutedEventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller != null && SelectedItem != null)
                controller.AddToWatchList(SelectedItem.Expression);
        }

        private void ContextMenu_AddAddressToWatchClick(object sender, RoutedEventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller != null && SelectedItem != null)
                controller.AddAddressToWatchList(SelectedItem.Expression);
        }

        private void ContextMenu_UnselectClick(object sender, RoutedEventArgs e)
        {
            listBox.UnselectAll();
        }

        private void ContextMenu_LargeThumbnailsClick(object sender, RoutedEventArgs e)
        {
            ScrollCurrentItemIntoView();
        }

        private void ContextMenu_DumpImageToFileClick(object sender, RoutedEventArgs e)
        {
            if (SelectedItem != null)
                ImageFileWriter.SaveFileDialog(SelectedItem.Image);
        }

        private void ContextMenu_CopyToClipboardClick(object sender, RoutedEventArgs e)
        {
            if (SelectedItem != null)
            {
                var bitmap = SelectedItem.Bitmap;
                if (bitmap != null)
                {
                    Clipboard.SetImage(bitmap);
                }
            }
        }
}

public class ValidSelectedItemToBooleanConverter :
        IValueConverter
    {
        public object Convert(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            var item = value as WatchListItem;

            return item != null && !string.IsNullOrEmpty(item.Expression)
                && item.IsValidExpression;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ValidInitializedSelectedItemToBooleanConverter :
        IValueConverter
    {
        public object Convert(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            var item = value as WatchListItem;

            return item != null && !string.IsNullOrEmpty(item.Expression)
                && item.IsValidExpression
                && item.Image != null && item.ImageIsInitialized;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ValidNonOperatorSelectedItemToBooleanConverter :
        IValueConverter
    {
        public object Convert(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            var item = value as WatchListItem;

            return item != null && !string.IsNullOrEmpty(item.Expression)
                && item.IsValidExpression && !item.IsOperatorExpression;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class WatchListItemForegroundBrushConverter :
        IValueConverter
    {
        public object Convert(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            if (!WPFHelpers.ValidateConverterInput(value))
                return null;

            bool isValidExpression = (bool)value;

            return isValidExpression ?
                MyToolWindow.FindResource("VsBrush.WindowText") :
                MyToolWindow.FindResource("VsBrush.GrayText");
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class WatchListSelectedItemBackgroundBrushConverter :
        IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            if (!WPFHelpers.ValidateConverterInput(values))
                return null;

            bool isSelectionActive = (bool)values[0];
            bool isValidExpression = (bool)values[1];
            bool isEditing = (bool)values[2];
            bool isContextMenuOpen = (bool)values[3] ||
                WatchListFocusManager.IsOtherContextMenuActive;

            if (isSelectionActive)
            {
                return isEditing ? MyToolWindow.FindResource("VsBrush.Window")
                    : MyToolWindow.FindResource("VsBrush.Highlight");
            }
            else if (isContextMenuOpen)
            {
                return MyToolWindow.FindResource("VsBrush.Highlight");
            }
            else
            {
                var c0 = (MyToolWindow.FindResource("VsBrush.AccentMedium")
                    as SolidColorBrush).Color;
                var c1 = (MyToolWindow.FindResource("ImageViewBackground")
                    as SolidColorBrush).Color;

                double alpha = 0.6;
                Color clr = Color.FromRgb(
                    (byte)(c0.R * alpha + (1 - alpha) * c1.R),
                    (byte)(c0.G * alpha + (1 - alpha) * c1.G),
                    (byte)(c0.B * alpha + (1 - alpha) * c1.B));

                return new SolidColorBrush(clr);
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class WatchListSelectedItemForegroundBrushConverter :
        IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            if (!WPFHelpers.ValidateConverterInput(values))
                return null;

            bool isSelectionActive = (bool)values[0];
            bool isValidExpression = (bool)values[1];
            bool isEditing = (bool)values[2];
            bool isContextMenuOpen = (bool)values[3] ||
                WatchListFocusManager.IsOtherContextMenuActive;

            if (isSelectionActive)
            {
                if (isEditing)
                {
                    return isValidExpression ?
                        MyToolWindow.FindResource("VsBrush.WindowText") :
                        MyToolWindow.FindResource("VsBrush.GrayText");
                }
                else
                {
                    return MyToolWindow.FindResource("VsBrush.HighlightText");
                }
            }
            else if (isContextMenuOpen)
            {
                return MyToolWindow.FindResource("VsBrush.HighlightText");
            }
            else
            {
                return isValidExpression ?
                    MyToolWindow.FindResource("VsBrush.WindowText") :
                    MyToolWindow.FindResource("VsBrush.GrayText");
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ItemStateBrushConverter :
       IValueConverter
    {
        public object Convert(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            return (bool)value ?
                new SolidColorBrush(Color.FromRgb(0, 0x78, 0xe6))
                : MyToolWindow.FindResource("ImageViewBackground");
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public static class WatchListFocusManager
    {
        // this is not beautiful ...
        public static bool IsOtherContextMenuActive = false;
    }
}