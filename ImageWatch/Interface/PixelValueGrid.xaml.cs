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
using System.Windows.Interop;

namespace Microsoft.ImageWatch.Interface
{
    /// <summary>
    /// Interaction logic for PixelValueGrid.xaml
    /// </summary>
    public partial class PixelValueGrid : UserControl
    {
        public PixelValueGrid()
        {
            InitializeComponent();
        }

        public void Clear()
        {
            labels_ = null;
            InvalidateVisual();
        }

        public Brush BackgroundBrush;

        private string[,,] labels_;
        private Vector offset_;
        private double scale_;
        
        private Geometry clipGeo_;
        public void Update(Size bmpSize, Vector offset, double scale, string[,,] labels)
        {
            clipGeo_ = new RectangleGeometry(new Rect(0, 0, bmpSize.Width, bmpSize.Height));

            offset_ = offset;
            scale_ = scale;
            labels_ = labels;
                        
            InvalidateVisual();
        }

        private static double GetTextSize(double scale)
        {
            return 6 * scale / 32;            
        }

        public static bool IsTextLargeEnough(double scale)
        {
            return GetTextSize(scale) >= 8;             
        }

        protected override void OnRender(DrawingContext dc)
        {
            if (labels_ == null)
                return;

            if (!IsTextLargeEnough(scale_))
                return;

            dc.PushClip(clipGeo_);

            double textSize = GetTextSize(scale_);
            double vtextSpacing = 1.1;

            int nb = labels_.GetLength(2);

            for (int i = 0; i < labels_.GetLength(0); ++i)
            {
                for (int j = 0; j < labels_.GetLength(1); ++j)
                {
                    Rect pixelRect = new Rect(offset_.X + j*scale_, 
                        offset_.Y + i*scale_, scale_, scale_);

                    // skip empty values
                    bool anyEmpty = false;
                    for (int r = 0; r < nb; ++r)
                    {
                        if (string.IsNullOrEmpty(labels_[i, j, r]))
                            anyEmpty = true;                 
                    }
                    if (anyEmpty)
                        continue;

                    List<FormattedText> text = new List<FormattedText>();                    
                    for (int r = 0; r < nb; ++r)
                    {
                        text.Add(new FormattedText(labels_[i, j, r],
                            System.Globalization.CultureInfo.CurrentCulture,
                            FlowDirection.LeftToRight, new Typeface("Consolas"),
                            textSize, new SolidColorBrush(Colors.Black), scale_));
                    }

                    double maxw = text.Max(t => t.Width);
                    double dx = (pixelRect.Width - maxw) / 2;
                    double dy = (pixelRect.Height - nb * textSize * vtextSpacing) / 2;

                    var tl = new Point(pixelRect.Left + dx, pixelRect.Top + dy);

                    double rectWidthDelta = 0.2;
                    dc.DrawRectangle(BackgroundBrush, null,
                        new Rect(tl.X - textSize * rectWidthDelta, tl.Y,
                            maxw + 2 * textSize * rectWidthDelta,
                            textSize * vtextSpacing * nb));
                    
                    for (int r = 0; r < nb; ++r)
                    {
                        double ddx = (maxw - text[r].Width) / 2;
                        dc.DrawText(text[r], new Point(tl.X + ddx, tl.Y));
                        tl.Y += textSize * vtextSpacing;
                    }
                }    
            }                           
        }
    }
}
