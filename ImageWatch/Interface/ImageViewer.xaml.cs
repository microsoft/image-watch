using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Globalization;

namespace Microsoft.ImageWatch.Interface
{
    /// <summary>
    /// Interaction logic for ImageViewer.xaml
    /// </summary>
    public partial class ImageViewer : UserControl
    {
        public ImageViewer()
        {
            InitializeComponent();

            canvas.SizeChanged += OnCanvasSizeChanged;

            // init status and bitmap info text
            OnItemViewChanged();
        }

        public void SetItem(WatchListItem item)
        {
            var oldItem = DataContext as WatchListItem;
            if (oldItem != null)
                oldItem.PropertyChanged -= OnItemPropertyChanged;

            DataContext = item;

            if (item != null)
                item.PropertyChanged += OnItemPropertyChanged;

            OnItemViewChanged();
        }

        void OnItemViewChanged()
        {
            SetInfoText();
            UpdateStatusBar();
            UpdateValueGrid();
        }

        private void UpdateValueGrid()
        { 
            var item = DataContext as WatchListItem;
            if (item == null || item.View == null || item.Image == null
                || item.Image.IsDisposed || !item.Image.HasPixelsLoaded()
                || !item.ImageHasNativePixelValues)
            {
                valueGrid.Clear();
                return;
            }

            double scale = item.GetViewTransformScale();

            if (!PixelValueGrid.IsTextLargeEnough(scale))
            {
                valueGrid.Clear();
                return;
            }

            Point ul = item.GetViewSourcePosFromTargetPos(new Point(0, 0));
            int tx = (int)Math.Floor(ul.X);
            int ty = (int)Math.Floor(ul.Y);
            Vector offset = new Vector(-(ul.X - tx) * scale, -(ul.Y - ty) * scale);

            Size vsz = new Size(item.View.PixelWidth, item.View.PixelHeight);
            int nx = (int) Math.Ceiling(vsz.Width / scale) + 1;
            int ny = (int) Math.Ceiling(vsz.Height / scale) + 1;
            
            int iw = (int)item.Image.GetInfo().Width;
            int ih = (int)item.Image.GetInfo().Height;                    

            var pf = item.Image.PixelFormat;
            int numDispBands = Math.Min(4, pf.NumBands);
            string[,,] labels = new string[ny, nx, numDispBands];
            for (int y = 0; y < ny; ++y)
            {
                int ry = ty + y;
                if (ry < 0 || ry >= ih)
                    continue;

                for (int x = 0; x < nx; ++x)
                {                
                    int rx = tx + x;
                    if (rx < 0 || rx >= iw)
                        continue;
                    
                    List<String> res = new List<string>();
                    var bytes = item.Image.ReadPixel(rx, ry);
                    for (int i = 0; i < numDispBands; ++i)
                        labels[y, x, i] = MakePixelValueString(bytes, i, pf, true);
                }
            }

            var bgc = item.BitmapBackground;
            bgc.A = 0xa0;
            valueGrid.BackgroundBrush = new SolidColorBrush(bgc);

            valueGrid.Update(vsz, offset, scale, labels);
        }

        void OnItemPropertyChanged(object sender, 
            PropertyChangedEventArgs e)
        {
            if (string.IsNullOrEmpty(e.PropertyName) ||
                e.PropertyName == "View")
            {
                OnItemViewChanged();
            }
        }

        private void SetInfoText()
        {
            if (DataContext == null)
            {
                bitmapInfo.Text = Microsoft.ImageWatch.Resources.NoSelectionText;
                bitmapInfo.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                bitmapInfo.SetBinding(TextBlock.TextProperty,
                    new Binding("ViewInfoString")
                    {
                        StringFormat = "[{0}]"
                    });

                bitmapInfo.SetBinding(TextBlock.VisibilityProperty,
                    new Binding("ViewInfoString")
                    {
                        Converter = new NullOrEmptyToVisibilityConverter()
                    });
            }
        }

        public event SizeChangedEventHandler CanvasSizeChanged;
        private void OnCanvasSizeChanged(object sender, 
            SizeChangedEventArgs e)
        {
            if (CanvasSizeChanged != null)
                CanvasSizeChanged(sender, e);
        }

        public Size ActualCanvasSize
        {
            get
            {
                return new Size(canvas.ActualWidth, canvas.ActualHeight);
            }
        }

        private void ContextMenu_ZoomToFitClick(object sender, 
            RoutedEventArgs e)
        {
            var item = DataContext as WatchListItem;
            if (item == null)
                return;

            item.ZoomViewToFit();
        }

        private void ContextMenu_ZoomToOriginalSizeClick(object sender,
            RoutedEventArgs e)
        {
            var item = DataContext as WatchListItem;
            if (item == null)
                return;

            item.ZoomViewToOriginalSize(lastRightClickPos_);
        }

        private void ContextMenu_CopyPixelAddressClick(object sender,
            RoutedEventArgs e)
        {
            var item = DataContext as WatchListItem;
            if (item == null)
                return;

            if (item.Image != null && item.ImageIsInitialized && 
                item.ImageHasNativePixelAddress &&
                lastRightClickPos_.HasValue)
            {
                var pos = item.GetViewSourcePosFromTargetPos(
                    lastRightClickPos_.Value);
                var addr = item.Image.GetInfo().PixelAddress + 
                    ((UInt64)pos.X) * item.Image.GetInfo().NumBytesPerPixel +
                    ((UInt64)pos.Y) * item.Image.GetInfo().NumStrideBytes;
                
                var len = addr > 0xffffffff ? "16" : "8";            
                Clipboard.SetText(string.Format("0x{0:x" + len + "}", addr));
            }
        }

        private void ContextMenu_LinkViewsChecked(object sender,
            RoutedEventArgs e)
        {
            // not beautiful ...
            if (MyToolWindow.TheController != null)
                MyToolWindow.TheController.LinkViews = true;
        }

        private void ContextMenu_LinkViewsUnchecked(object sender,
            RoutedEventArgs e)
        {
            // not beautiful ...
            if (MyToolWindow.TheController != null)
                MyToolWindow.TheController.LinkViews = false; 
        }

        private void ContextMenu_HexDisplayChecked(object sender,
           RoutedEventArgs e)
        {
            var dte = ImageWatchPackage.DTE;
            if (dte == null || dte.Debugger == null)
                return;

            dte.Debugger.HexDisplayMode = true;

            UpdateStatusBar();
            UpdateValueGrid();
        }

        private void ContextMenu_HexDisplayUnchecked(object sender,
            RoutedEventArgs e)
        {
            var dte = ImageWatchPackage.DTE;
            if (dte == null || dte.Debugger == null)
                return;

            dte.Debugger.HexDisplayMode = false;

            UpdateStatusBar();
            UpdateValueGrid();
        }

        private void ContextMenu_Opening(object sender, RoutedEventArgs e)
        {
            var dte = ImageWatchPackage.DTE;
            if (dte != null && dte.Debugger != null)
                hexDisplay.IsChecked = dte.Debugger.HexDisplayMode;

            WatchListFocusManager.IsOtherContextMenuActive = true;

            maximizeContrastCheck.IsChecked =
                MyToolWindow.TheController.AutoScaleColormap;
            pseudoColorCheck.IsChecked =
                MyToolWindow.TheController.ColormapJet;
            alphaColorCheck.IsChecked =
                MyToolWindow.TheController.FourChannelIgnoreAlpha;
        }

        private void ContextMenu_Closing(object sender, RoutedEventArgs e)
        {
            WatchListFocusManager.IsOtherContextMenuActive = false;
            
            // menu may be closed with mouse outside of viewer. In that case
            // no MouseMove is triggered and so we have to manually make 
            // sure the status bar is cleared 
            UpdateStatusBar();
        }

        private void ContextMenu_ColormapJetChecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.ColormapJet = true;
        }

        private void ContextMenu_ColormapJetUnchecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.ColormapJet = false;
        }

        private void ContextMenu_AutoMaximizeChecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.AutoScaleColormap = true;
        }

        private void ContextMenu_AutoMaximizeUnchecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.AutoScaleColormap = false;
        }

        private void ContextMenu_FourChannelIgnoreAlphaChecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.FourChannelIgnoreAlpha = true;
        }

        private void ContextMenu_FourChannelIgnoreAlphaUnchecked(object sender,
            RoutedEventArgs e)
        {
            MyToolWindow.TheController.FourChannelIgnoreAlpha = false;
        }

        private void viewImage_MouseWheel(object sender, 
            MouseWheelEventArgs e)
        {
            if (isPanning_)
                return;

            if (e.Delta == 0)
                return;

            var item = DataContext as WatchListItem;
            if (item == null)
                return;

            item.ZoomViewAtPosition(e.GetPosition(viewImage), 
                //Math.Exp(e.Delta * 0.002));
                Math.Pow(2, 0.333333 * (e.Delta/120.0)));
        }

        Point? lastRightClickPos_;
        private void viewImage_MouseRightButtonDown(object sender,
            MouseButtonEventArgs e)
        {
            lastRightClickPos_ = e.GetPosition(viewImage);
        }

        bool isPanning_ = false;
        Vector centerTargetRef_;
        private void viewImage_MouseLeftButtonDown(object sender,
           MouseButtonEventArgs e)
        {
            if (WPFHelpers.CtrlPressed())
                return;

            var item = DataContext as WatchListItem;
            if (item == null)
                return;

            centerTargetRef_ = item.GetViewTransformCenterTarget() - 
                e.GetPosition(viewImage);

            // Note: this triggers a MouseMove event and should be called at 
            // the end of the LeftButtonDown handler
            viewImage.CaptureMouse();

            if (viewImage.IsMouseCaptured)
                isPanning_ = true;
        }

        private static string SignedIntString(Int64 v, int width)
        {
            return string.Format("{0}{1:d" + width.ToString() + "}", 
                (v > 0 ? "+" : ""), v);
        }

        /*private static string FormatRealValue(float v)
        {
            string signspace = v > 0 ? "+" : "";
            return string.Format("{0}{1:0.00e+00}", signspace, v);
        }*/
        private static string FormatRealValue(double v, bool forceShort)
        {
            if (forceShort)
            {
                if (v == 0)
                    return "0";

                if (Math.Abs(v) > 1e99)
                    return string.Format("{0:g2}", v);

                if (Math.Abs(v) < 0.01)
                {
                    return v.ToString("0.##e-00", CultureInfo.InvariantCulture);
                }
                else if (Math.Abs(v) < 1)
                {
                    return v.ToString("G4");
                }
                else if (Math.Abs(v) <= 10000)
                {
                    return v.ToString("G5");
                }
                else if (Math.Abs(v) <= 99999)
                {
                    return v.ToString("G3");
                }
                else
                {
                    return v.ToString("0.00e+00", CultureInfo.InvariantCulture);
                }
            }
            else
            {
                return string.Format("{0:g5}", v);
            }
        }

        private static string
            RawPixelBytesToString(byte[] bytes, int offset, string format,
            bool forceShort)
        {
            string res = "";

            switch (format)
            {
                case "BYTE":
                    if (bytes.Length - offset >= 1)
                    {
                        byte v = bytes[offset];
                        if (forceShort)
                            res = string.Format("{0}", v);
                        else
                            res = string.Format("{0:d3}", v);
                    }
                    break;
                case "SBYTE":
                    if (bytes.Length - offset >= 1)
                    {
                        sbyte v = (sbyte)bytes[offset];
                        if (forceShort)
                            res = string.Format("{0}", v);
                        else
                            res = SignedIntString(v, 3);
                    }
                    break;
                case "SHORT":
                    if (bytes.Length - offset >= 2)
                    {
                        ushort v = BitConverter.ToUInt16(bytes, offset);
                        if (forceShort)
                            res = string.Format("{0}", v);
                        else
                            res = string.Format("{0:d5}", v);
                    }
                    break;
                case "SSHORT":
                    if (bytes.Length - offset >= 2)
                    {
                        short v = BitConverter.ToInt16(bytes, offset);
                        if (forceShort)
                            res = string.Format("{0}", v);
                        else
                            res = SignedIntString(BitConverter.ToInt16(bytes, offset), 5);
                    }
                    break;
                case "INT":
                    if (bytes.Length - offset >= 4)
                    {
                        int v = BitConverter.ToInt32(bytes, offset);
                        if (forceShort)
                            res = FormatRealValue(v, true);
                        else 
                            res = SignedIntString(v, 10);
                    }
                    break;
                case "HALF_FLOAT":
                    if (bytes.Length - offset >= 2)
                        res = FormatRealValue(
                            Microsoft.Research.NativeImage.NativeImageHelpers
                            .HalfFloatRawBytesToFloat(bytes, offset), 
                            forceShort);
                    break;
                case "FLOAT":
                    if (bytes.Length - offset >= 4)
                        res = FormatRealValue(
                            BitConverter.ToSingle(bytes, offset), forceShort);
                    break;
                case "DOUBLE":
                    if (bytes.Length - offset >= 8)
                        res = FormatRealValue(
                            BitConverter.ToDouble(bytes, offset), forceShort);
                    break;
            }

            return res;
        }

        private static string
            RawPixelBytesToHexString(byte[] bytes, int offset, string format,
            bool skip0x = false)
        {
            string res = "";

            string fmthdr = skip0x ? "" : "0x";

            switch (format)
            {
                case "BYTE":
                case "SBYTE":
                    if (bytes.Length - offset >= 1)
                        res = string.Format(fmthdr + "{0:x2}", bytes[offset]);
                    break;
                case "SHORT":
                case "SSHORT":
                case "HALF_FLOAT":
                    if (bytes.Length - offset >= 2)
                        res = string.Format(fmthdr + "{0:x4}",
                            BitConverter.ToUInt16(bytes, offset));
                    break;
                case "INT":
                case "FLOAT":
                    if (bytes.Length - offset >= 4)
                        res = string.Format(fmthdr + "{0:x8}",
                            BitConverter.ToUInt32(bytes, offset));
                    break;
                case "DOUBLE":
                    if (bytes.Length - offset >= 8)
                        res = string.Format(fmthdr + "{0:x16}",
                            BitConverter.ToUInt64(bytes, offset));
                    break;
            }

            return res;
        }
        private void ClearPixelToolTipRow()
        {
            pixelToolTipGrid.Children.Clear();
            pixelToolTipGrid.RowDefinitions.Clear();            
        }

        private void AddPixelToolTipSeparator()
        {
            pixelToolTipGrid.RowDefinitions.Add(new RowDefinition() 
            {
                Height = new GridLength(5)
            });
        }

        private void AddPixelToolTipRow(string c0, string c1 = null)
        {
            pixelToolTipGrid.RowDefinitions.Add(new RowDefinition());

            TextBlock tb0 = new TextBlock();
            tb0.Text = c0;
            tb0.HorizontalAlignment = HorizontalAlignment.Left;
            pixelToolTipGrid.Children.Add(tb0);
            Grid.SetRow(tb0, pixelToolTipGrid.RowDefinitions.Count - 1);

            if (string.IsNullOrEmpty(c1))
            {
                Grid.SetColumnSpan(tb0, 
                    pixelToolTipGrid.ColumnDefinitions.Count); 
            }
            else
            {
                Grid.SetColumn(tb0, 0);
                TextBlock tb1 = new TextBlock();
                tb1.HorizontalAlignment = HorizontalAlignment.Right;
                tb1.Text = c1;
                Grid.SetColumn(tb1, 
                    pixelToolTipGrid.ColumnDefinitions.Count - 1);
                pixelToolTipGrid.Children.Add(tb1);
                Grid.SetRow(tb1, pixelToolTipGrid.RowDefinitions.Count - 1);
            }           
        }

        private static string MakePixelValueString(byte[] bytes, int offset,
            Microsoft.Research.NativeImage.NativePixelFormat pf,
            bool forceShort = false)
        {
            bool hexMode = false;
            var dte = ImageWatchPackage.DTE;
            hexMode = dte != null && dte.Debugger != null && 
                dte.Debugger.HexDisplayMode;

            if (!hexMode || (forceShort && pf.NumBytesPerElement > 2))
            {
                return string.Format("{0}", RawPixelBytesToString(bytes,
                    offset * pf.NumBytesPerElement, pf.ElementFormat, 
                    forceShort));
            }
            else 
            {
                return string.Format("{0}", 
                    RawPixelBytesToHexString(bytes,
                    offset * pf.NumBytesPerElement, pf.ElementFormat));
            }
        }

        private void PopulatePixelToolTip()
        {
            ClearPixelToolTipRow();

            if (!IsMouseOverValidPixel())
                return;

            var item = DataContext as WatchListItem;
            if (item == null)
                return;
            
            var srcPos = item.GetViewSourcePosFromTargetPos(
                Mouse.GetPosition(viewImage));
            AddPixelToolTipRow("x:", string.Format("{0}", (int)srcPos.X));
            AddPixelToolTipRow("y:", string.Format("{0}", (int)srcPos.Y));
            
            AddPixelToolTipSeparator();

            var pv = FormatPixelValues(item, srcPos);
            if (pv == null)
            {
                AddPixelToolTipRow("[n/a]");
                return;
            }

            for (int i = 0; i < pv.Length; ++i)
            {
                if (i > 9)
                {
                    AddPixelToolTipRow("...");
                    break;
                }
                
                AddPixelToolTipRow(string.Format("{0}:", i), pv[i]);
            }

           /* if (item.Image != null && item.ImageIsInitialized &&
                item.ImageHasNativePixelAddress)
            {
                AddPixelToolTipSeparator();

                var addr = item.Image.GetInfo().PixelAddress +
                    ((UInt64)srcPos.X) * item.Image.GetInfo().NumBytesPerPixel +
                    ((UInt64)srcPos.Y) * item.Image.GetInfo().NumStrideBytes;
                
                var len = addr > 0xffffffff ? "16" : "8";
                AddPixelToolTipRow(string.Format("0x{0:x" + len + "}", addr));
            }*/
        }

        private static string[] FormatPixelValues(WatchListItem item,
            Point srcPos)
        {
            if (item == null || item.Image == null
                || item.Image.IsDisposed || !item.Image.HasPixelsLoaded()
                || !item.ImageHasNativePixelValues)
                return null;

            List<string> res = new List<string>();
            var pf = item.Image.PixelFormat;
            var bytes = item.Image.ReadPixel((int)srcPos.X, (int)srcPos.Y);
            for (int i = 0; i < pf.NumBands; ++i)
                res.Add(MakePixelValueString(bytes, i, pf));

            return res.ToArray();
        }
        
        private void viewImage_MouseMove(object sender, MouseEventArgs e)
        {
            var curMousePos = e.GetPosition(viewImage);

            if (isPanning_)
            {
                var item = DataContext as WatchListItem;
                if (item == null)
                    return;

                var correction =
                    item.SetViewTransformCenterTarget(centerTargetRef_ +
                    curMousePos);

                centerTargetRef_ += correction;
            }

            UpdateToolTipVisibility();
                        
            if (pixelToolTip.IsOpen)
            {
                PopulatePixelToolTip();

                if (popupOffset.HasValue)
                {
                    pixelToolTip.MoveTo(
                        new Point(popupOffset.Value.X + curMousePos.X,
                        popupOffset.Value.Y + curMousePos.Y));
                }                
            }

            UpdateStatusBar();
        }

        private void UpdateToolTipVisibility()
        {
            if (IsMouseOverValidPixel() && CtrlPressed())
            {
                // disabled for now... 
                // pixelToolTip.IsOpen = true;
            }
            else
            {
                pixelToolTip.IsOpen = false;
            }
        }

        private void viewImage_MouseLeftButtonUp(object sender,
            MouseButtonEventArgs e)
        {
            viewImage.ReleaseMouseCapture();
        }
 
        private void viewImage_LostMouseCapture(object sender, 
            MouseEventArgs e)
        {
            isPanning_ = false;
        }

        private void viewImage_MouseLeave(object sender, MouseEventArgs e)
        {
            pixelToolTip.IsOpen = false;

            // clear (forced) when leaving the window. Need to force, because
            // at the time of leaving the mouse position is sometimes still 
            // over the image.
            // NOTE: if we leave because of context menu opening, we should
            // not clear! context menu events: 1) ctxmenu_opening, 2) mouseleave
            // -> only clear status bar if leaving "while" opening. The close
            // part of this is UpdateStatusBar() in the ctxmenu_closing 
            // callback
            if (!viewContextMenu.IsOpen)
                UpdateStatusBar(true);
        }

        public void ParentKeyDown(int vkey)
        {
            if (vkey == 0x11)
			{
				UpdateToolTipVisibility();          
				if (pixelToolTip.IsOpen)
					PopulatePixelToolTip();
			}
        }

        public void ParentKeyUp(int vkey)
        {
            if (vkey == 0x11)
                pixelToolTip.IsOpen = false;
        }

        private bool CtrlPressed()
        {
            return (Keyboard.IsKeyDown(Key.LeftCtrl) ||
                Keyboard.IsKeyDown(Key.RightCtrl));
        }
        
        private bool IsMouseOverViewImage()
        {
            return VisualTreeHelper.HitTest(viewImage,
                Mouse.GetPosition(viewImage)) != null;
        }

        private bool IsMouseOverValidPixel()
        {
            var item = DataContext as WatchListItem;
            if (item == null)
                return false;

            return IsMouseOverViewImage() &&
                item.IsViewTargetPointInsideImage(
                Mouse.GetPosition(viewImage));
        }

        private Vector? popupOffset;
        private void pixelToolTip_Opened(object sender, EventArgs e)
        {
            popupOffset = pixelToolTip.Position - 
                Mouse.GetPosition(viewImage);
        }

        private List<Run> MakeStatusBarPixelTextRuns()
        {
            var item = DataContext as WatchListItem;
            if (item == null)
                return new List<Run>();

            List<KeyValuePair<bool, string>> res = 
                new List<KeyValuePair<bool, string>>();

            var srcPos = item.GetViewSourcePosFromTargetPos(
                Mouse.GetPosition(viewImage));

            //res.Add(new KeyValuePair<bool, string>(false, "x:"));
            res.Add(new KeyValuePair<bool, string>(true, 
                string.Format("{0:d4}", (int)srcPos.X)));
            res.Add(new KeyValuePair<bool, string>(false, " "));
            //res.Add(new KeyValuePair<bool, string>(false, "y:"));
            res.Add(new KeyValuePair<bool, string>(true,
                string.Format("{0:d4}", (int)srcPos.Y)));
            
            var pv = FormatPixelValues(item, srcPos);
            if (pv != null)
            {
                res.Add(new KeyValuePair<bool, string>(false, " | "));

                for (int i = 0; i < pv.Length; ++i)
                {
                   // res.Add(new KeyValuePair<bool, string>(false, 
                     //   string.Format("{0}:", i)));
                    res.Add(new KeyValuePair<bool, string>(true, pv[i]));
                    
                    if (i < pv.Length - 1)
                        res.Add(new KeyValuePair<bool, string>(false, " "));
                }
            }

            var fgb = MyToolWindow.FindResource("VsBrush.WindowText") as Brush;
            var bgb = MyToolWindow.FindResource("VsBrush.GrayText") as Brush;

            return res.Select(r => new Run(r.Value) 
            { 
                Foreground = r.Key ? fgb : bgb
            }).ToList();
        }

        private void UpdateStatusBar(bool mouseLeaving = false)
        {
            var item = DataContext as WatchListItem;
            if (item == null)
            {
                statusText.Inlines.Clear();
                zoomText.Text = "";
                return;
            }

            if (item.View != null)
            {
                zoomText.Text = string.Format("{0:0.00}x",
                    item.GetViewTransformScale());
            }
            else
            {
                zoomText.Text = null;
            }

            if (mouseLeaving || !IsMouseOverValidPixel())
            {
                statusText.Inlines.Clear();
            }
            else
            {
                statusText.Inlines.Clear();
                statusText.Inlines.AddRange(MakeStatusBarPixelTextRuns());
            }
        }
    }
}
