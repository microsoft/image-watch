using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Microsoft.ImageWatch.Interface
{
    /// <summary>
    /// Interaction logic for MyControl.xaml
    /// </summary>
    public partial class MainControl : UserControl
    {
        WatchListItem localsSelectedItemBeforeUpdate_ = null;

        public MainControl()
        {
            InitializeComponent();

            IsVisibleChanged += MainControl_IsVisibleChanged;
            DataContextChanged += MainControl_DataContextChanged;

            localsListControl.SelectedItemChanged += localsListControl_SelectedItemChanged;
            watchListControl.SelectedItemChanged += watchListControl_SelectedItemChanged;
            localsRadio.Checked += localsRadio_Checked;
            watchRadio.Checked += watchRadio_Checked;

            viewer.CanvasSizeChanged += canvas_SizeChanged;
            viewer.MouseDown += viewer_MouseDown;

            MouseLeftButtonDown += MainControl_MouseLeftButtonDown;
            PreviewMouseRightButtonDown += MainControl_PreviewMouseRightButtonDown;
        }

        void viewer_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left && WPFHelpers.CtrlPressed())
            {
                var controller = DataContext as Controller;
                if (controller == null)
                    return;

                (controller.LocalsSelected ? localsListControl : watchListControl)
                    .SelectLastSelectedItem();
            }
        }

        void MainControl_PreviewMouseRightButtonDown(object sender, 
            MouseButtonEventArgs e)
        {
            localsListControl.MainControl_PreviewMouseRightButtonDown(sender, e);
            watchListControl.MainControl_PreviewMouseRightButtonDown(sender, e);
        }

        void MainControl_MouseLeftButtonDown(object sender, 
            MouseButtonEventArgs e)
        {
            localsListControl.MainControl_MouseLeftButtonDown(sender, e);
            watchListControl.MainControl_MouseLeftButtonDown(sender, e);
        }

        public void ParentKeyDown(int vkey)
        {
            viewer.ParentKeyDown(vkey);
        }

        public void ParentKeyUp(int vkey)
        {
            viewer.ParentKeyUp(vkey);
        }

        void canvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            if (viewer.ActualCanvasSize.Width > 0 &&
                viewer.ActualCanvasSize.Height > 0)
            {
                controller.InitializeView(
                    (int)viewer.ActualCanvasSize.Width,
                    (int)viewer.ActualCanvasSize.Height);
            }
        }
        
        void AttachViewerToCurrentItem()
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            var vi = (controller.LocalsSelected ? 
                localsListControl: watchListControl).NonNullSelectedItem;
            
            controller.AttachViewToItem(vi);

            viewer.SetItem(vi);
        }
               
        void MainControl_DataContextChanged(object sender,
            DependencyPropertyChangedEventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            controller.WatchAdded += (s, ea) =>
                {
                    var args = ea as WatchAddedEventArgs;
                    if (ea != null)
                        watchListControl.SelectedItem = args.Item;

                    if (controller != null)
                        controller.LocalsSelected = false;
                };

            controller.PreLocalsUpdated += (s, ea) =>
                {
                    localsSelectedItemBeforeUpdate_ =
                        localsListControl.SelectedItem;
                };

            controller.LocalsUpdated += (s, ea) =>
                {
                    localsListControl.SelectedItem =
                        localsSelectedItemBeforeUpdate_;
                };
            
            AttachViewerToCurrentItem();
        }

        void localsListControl_SelectedItemChanged(object sender, EventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            if (controller.LocalsSelected)
                AttachViewerToCurrentItem();
        }

        void watchListControl_SelectedItemChanged(object sender, EventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            if (!controller.LocalsSelected)
                AttachViewerToCurrentItem();
        }

        void watchRadio_Checked(object sender, RoutedEventArgs e)
        {
            AttachViewerToCurrentItem();
        }

        void localsRadio_Checked(object sender, RoutedEventArgs e)
        {
            AttachViewerToCurrentItem();
        }

        void MainControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            var controller = DataContext as Controller;
            if (controller == null)
                return;

            if (controller.WatchList != null)
            {
                if (watchListControl.ItemsSource != controller.WatchList)
                    watchListControl.ItemsSource = controller.WatchList;
            }

            if (controller.LocalsList != null)
            {
                if (localsListControl.ItemsSource != controller.LocalsList)
                    localsListControl.ItemsSource = controller.LocalsList;
            }
        }

        private void HelpButton_Click(object sender, RoutedEventArgs e)
        {
            var url = ImageWatchPackage.Config.HelpPageURL;

            if (!string.IsNullOrWhiteSpace(url))
            {
                if (!url.StartsWith("http") && !System.IO.File.Exists(url))
                    url = ImageWatchPackage.Config.DefaultHelpPageURL;
                
                Process.Start(url);
            }
        }
    }    
}