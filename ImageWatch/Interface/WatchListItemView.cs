using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Interop;
using System.Windows;
using System.Windows.Media;

using Microsoft.Research.NativeImage;

namespace Microsoft.ImageWatch.Interface
{
    public class WatchListItemView : BindableBase, IDisposable
    {
        private Microsoft.Research.NativeImage.NativeImageView view_ = null;

        private void InitalizeView(bool enableGrid)
        {
            var oldView = view_;
            view_ = null;

            view_ = new Research.NativeImage.NativeImageView(width_, height_);
            
            view_.BackgroundColor = backgroundColor_;            
            view_.GridEnabled = enableGrid;

            view_.SetSource(image_);

            if (oldView != null)
                oldView.Dispose();
        }

        public event EventHandler SizeChanged;

        private int width_ = 0;
        private int height_ = 0;
        private Color backgroundColor_ = Colors.Black;
        public void Initialize(int width, int height, Color backgroundColor,
            bool enableZoomInGrid)
        {
            if (width_ != width || height_ != height || 
                backgroundColor_ != backgroundColor)
            {
#if DEBUG
                System.Diagnostics.Debug.WriteLine("---reinitializing view");
#endif

                width_ = width;
                height_ = height;
                backgroundColor_ = backgroundColor;

                InitalizeView(enableZoomInGrid);
                                
                if (SizeChanged != null)
                    SizeChanged(this, EventArgs.Empty);
            }                
        }

        private WatchedImage image_;
        public void SetImage(WatchedImage image)
        {
            image_ = image;

            // Caution: SetSource() will always reset to default transform
            // and mark view as dirty
            if (view_ != null && view_.GetSource() != image_)
                view_.SetSource(image_);
        }

        public void MarkAsDirty()
        {
            if (view_ != null)
                view_.MarkAsDirty();
        }

        public void EnableGrid(bool value)
        {
            if (view_ != null)
                view_.GridEnabled = value;
        }

        public Point TransformCenter
        {
            get
            {
                if (view_ == null)
                    return new Point();

                return view_.BitmapToSourceXY(new Point(width_ * 0.5,
                    height_ * 0.5));
            }
        }

        public Point TransformCenterTarget
        {
            get
            {
                if (view_ == null || image_ == null || image_.IsDisposed)
                    return new Point();

                return view_.SourceToBitmapXY(new Point(
                    image_.Rectangle.X + image_.Rectangle.Width * 0.5,
                    image_.Rectangle.Y + image_.Rectangle.Height * 0.5));
            }
        }
        
        public double TransformScale
        {
            get
            {
                if (view_ == null)
                    return 1.0;

                return view_.Scale;
            }
        }

        public Size BitmapSize
        {
            get
            {
                return view_ != null ? new Size(view_.PixelWidth,
                    view_.PixelHeight) : Size.Empty;
            }
        }

        public InteropBitmap Bitmap
        {
            get
            {
                RenderIfDirty();
                
                if (view_ != null && 
                    image_ != null && !image_.IsDisposed &&
                    string.IsNullOrWhiteSpace(view_.LastError))
                {                    
                    return view_.Bitmap;
                }
                else
                {
                    return null;
                }
            }
        }
        
        public double NearScale = 64.0;

        public double FitScaleFactor = 1.0;        
                
        private double FitScale
        {
            get
            {
                if (image_ != null && !image_.IsDisposed)
                {
                    return FitScaleFactor * Math.Min(
                        width_ / (double)image_.Rectangle.Width,
                        height_ / (double)image_.Rectangle.Height);
                }

                return 1.0;
            }
        }
        
        private double FarScale
        {
            get
            {
                return Math.Min(FitScale, 1.0);
            }
        }

        public void SetDefaultTransform()
        {
            if (view_ != null && image_ != null && !image_.IsDisposed)
            {
                view_.SourceRef = new Point(
                    image_.Rectangle.X + image_.Rectangle.Width * 0.5,
                    image_.Rectangle.Y + image_.Rectangle.Height * 0.5);
                view_.BitmapRef = new Point(width_ * 0.5, height_ * 0.5);
                view_.Scale = FitScale;
            }

            ClampTransform();
        }

        public Vector SetTransformScale(double scale)
        {
            if (view_ != null)
                view_.Scale = scale;

            return ClampTransform();
        }

        // set the point in the source image that maps to bitmap center
        public Vector SetTransformCenterAndScale(Point center, double scale)
        {
            if (view_ != null)
            {
                view_.BitmapRef = new Point(width_ * 0.5, height_ * 0.5);
                view_.SourceRef = center;
                view_.Scale = scale;
            }

            return ClampTransform();
        }

        // set the point in the bitmap that source center maps to
        public Vector SetTransformCenterTarget(Point centerTarget)
        {
            if (view_ != null && image_ != null && !image_.IsDisposed)
            {
                view_.BitmapRef = centerTarget;
                view_.SourceRef = new Point(
                    image_.Rectangle.X + image_.Rectangle.Width * 0.5,
                    image_.Rectangle.Y + image_.Rectangle.Height * 0.5);
            }

            return ClampTransform();
        }

        public void ZoomInAt(Point bitmapPos, double factor)
        {
            if (view_ != null)
            {
                var sourcePos = view_.BitmapToSourceXY(bitmapPos);

                view_.BitmapRef = bitmapPos;
                view_.SourceRef = sourcePos;
                view_.Scale = view_.Scale * factor;
            }

            ClampTransform();
        }
        
		// clamp transform to "sensisble" range. returns correction 
		// translation vector 
        private Vector ClampTransform()
        {
            if (view_ == null || image_ == null || image_.IsDisposed)
                return new Vector(0, 0);

            // constrain transform so that a significant portion of the 
            // image is visible at all times

            view_.Scale = Math.Max(FarScale, Math.Min(NearScale,
                view_.Scale));

            double maxXBorder = Math.Max(0.0, 0.5 * (width_ -
                image_.Rectangle.Width * FarScale));
            double maxYBorder = Math.Max(0.0, 0.5 * (height_ -
                image_.Rectangle.Height * FarScale));

            Point sourceTopLeft = new Point(image_.Rectangle.X,
                image_.Rectangle.Y);
            Point sourceBottomRight = new Point(image_.Rectangle.X
                + image_.Rectangle.Width, image_.Rectangle.Y
                + image_.Rectangle.Height);

            Point mappedSourceTopLeft =
                view_.SourceToBitmapXY(sourceTopLeft);
            Point mappedSourceBottomRight =
                view_.SourceToBitmapXY(sourceBottomRight);

            double eps = 0.01;
            double deltaX = 0.0;
            double deltaY = 0.0;
            if (mappedSourceTopLeft.X > eps &&
                mappedSourceTopLeft.X > maxXBorder + eps)
            {
                deltaX = maxXBorder - mappedSourceTopLeft.X;
            }
            else if (mappedSourceBottomRight.X < width_ - eps &&
                mappedSourceBottomRight.X < width_ - maxXBorder - eps)
            {
                deltaX = width_ - maxXBorder -
                    mappedSourceBottomRight.X;
            }

            if (mappedSourceTopLeft.Y > eps &&
                mappedSourceTopLeft.Y > maxYBorder + eps)
            {
                deltaY = maxYBorder - mappedSourceTopLeft.Y;
            }
            else if (mappedSourceBottomRight.Y < height_ - eps &&
                mappedSourceBottomRight.Y < height_ - maxYBorder - eps)
            {
                deltaY = height_ - maxYBorder -
                    mappedSourceBottomRight.Y;
            }

            view_.BitmapRef = new Point(view_.BitmapRef.X + deltaX,
                view_.BitmapRef.Y + deltaY);

            return new Vector(deltaX, deltaY);
        }

        public bool IsTargetPointInsideSource(Point targetXY)
        {
            if (view_ == null || image_ == null || image_.IsDisposed)
                return false;

            var xy = view_.BitmapToSourceXY(targetXY);

            return xy.X >= image_.Rectangle.X && 
                xy.X < image_.Rectangle.X + image_.Rectangle.Width && 
                xy.Y >= image_.Rectangle.Y 
                && xy.Y < image_.Rectangle.Y + image_.Rectangle.Height;
        }

        public Point GetSourcePosFromTargetPos(Point targetXY)
        {
            return view_ != null ? view_.BitmapToSourceXY(targetXY) :
                new Point(0, 0);
        }

        double colorMapDomainStart_ = 0.0;
        double colorMapDomainEnd_ = 1.0;
        public void SetColormapDomain(double start, double end)
        {
            colorMapDomainStart_ = start;
            colorMapDomainEnd_ = end;
        }

        public void SetDefaultColormapDomain()
        { 
            if (image_ == null || image_.IsDisposed)
            {
                colorMapDomainStart_ = 0.0;
                colorMapDomainEnd_ = 1.0;
                return;
            }

            var name = Microsoft.Research.NativeImage.ColorMapManager
                .GetDefaultMapName(image_);

            colorMapDomainStart_ = 
                ColorMapManager.GetMapDomainStart(name, image_);
            colorMapDomainEnd_ = 
                ColorMapManager.GetMapDomainEnd(name, image_);
        }

        bool colormapJet_ = false;
        public void SetColormapJet(bool value)
        {
            colormapJet_ = value;
        }

        bool fourChannelsIgnoreAlpha_ = false;
        public void SetColormapFourChannelsIgnoreAlpha(bool value)
        {
            fourChannelsIgnoreAlpha_ = value;
        }

        private void RenderIfDirty()
        {
            if (view_ != null)
            {                
                if (image_ != null && !image_.IsDisposed)
                {
                    if (colormapJet_ && image_.PixelFormat.NumBands == 1)
                    {
                        view_.ColorMapName = "Matlab Jet";
                    }
                    else if (fourChannelsIgnoreAlpha_ && 
                        image_.PixelFormat.NumBands == 4)
                    {
                        view_.ColorMapName = "Band 0-2 BGR";
                    }
                    else
                    {
                        if (!string.IsNullOrEmpty(image_
                            .GetInfo().SpecialColorMapName))
                        {
                            view_.ColorMapName =
                                image_.GetInfo().SpecialColorMapName;
                        }
                        else
                        {
                            view_.ColorMapName = 
                                Microsoft.Research.NativeImage
                                .ColorMapManager.GetDefaultMapName(image_);
                        }
                    }

                    view_.ColorMapSelectedBand = -1;

                    view_.ColorMapDomainStart = colorMapDomainStart_;
                    view_.ColorMapDomainEnd = colorMapDomainEnd_;
                                                            
                    view_.Render();
                }
                else
                {
                    view_.Clear();
                }

                if (view_.Bitmap != null)
                    view_.Bitmap.Invalidate();
            }
        }

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (view_ != null)
                {
                    view_.Dispose();
                    view_ = null;
                }
            }
        }
    }
}
