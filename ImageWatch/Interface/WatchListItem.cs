using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;

namespace Microsoft.ImageWatch.Interface
{
    public class WatchListItem : BindableBase, IDisposable
    {
        public WatchListItem(string watchExpression)
        {
            Expression = watchExpression;
        }

        private bool isEditing_ = false;
        public bool IsEditing
        {
            get
            {
                return isEditing_;
            }
            set
            {
                SetProperty(ref isEditing_, value);
            }
        }

        private bool isExpanded_ = false;
        public bool IsExpanded
        {
            get
            {
                return isExpanded_;
            }
            set
            {
                bool wasExpanded = isExpanded_;
                SetProperty(ref isExpanded_, value);
                if (!wasExpanded && isExpanded_)
                    NotifyAllPropertiesChanged();
            }
        }

        public bool IsActive { get; set; }

        public bool IsInScope { get; set; } = true;

        private bool isContextOutOfDate_ = true;
        public bool IsContextOutOfDate
        {
            get
            {
                return isContextOutOfDate_;
            }
            private set
            {
                SetProperty(ref isContextOutOfDate_, value);
            }
        }

        public void MarkContextOutOfDate()
        {
            IsContextOutOfDate = true;
        }

        private bool isExpressionOutOfDate_ = true;
        public bool IsExpressionOutOfDate
        {
            get
            {
                return isExpressionOutOfDate_;
            }
            private set
            {
                SetProperty(ref isExpressionOutOfDate_, value);

                if (value)
                    IsInfoOutOfDate = value;
            }
        }

        public void MarkExpressionOutOfDate()
        {
            IsExpressionOutOfDate = true;
        }

        private bool isInfoOutOfDate_ = true;
        public bool IsInfoOutOfDate
        {
            get
            {
                return isInfoOutOfDate_;
            }
            private set
            {
                SetProperty(ref isInfoOutOfDate_, value);

                if (value)
                {
                    // reset properties on image, since these are forwarded
                    if (image_ != null)
                        image_.InvalidateInfo();

                    ArePixelsOutOfDate = value;
                }
            }
        }

        public void MarkInfoOutOfDate()
        {
            IsInfoOutOfDate = true;
        }

        private bool arePixelsOutOfDate_ = true;
        public bool ArePixelsOutOfDate
        {
            get
            {
                return arePixelsOutOfDate_;
            }

            private set
            {
                SetProperty(ref arePixelsOutOfDate_, value);

                if (value)
                {
                    if (thumb_ != null)
                        thumb_.MarkAsDirty();
                    if (view_ != null)
                        view_.MarkAsDirty();

                    imageMinMaxOutOfDate = true;
                }
            }
        }

        private ExpressionInfo expressionInfo_ = null;

        private void DebugLogAction(string action)
        {
#if DEBUG
            System.Diagnostics.Debug.WriteLine("  * '{0}': {1}",
                new object[] { expression_, action });
#endif
        }

        private void UpdateExpression(bool invalidateFirst)
        {
            if (invalidateFirst)
                IsExpressionOutOfDate = true;

            DebugLogAction("update expression");

            expressionInfo_ = ExpressionHelper.ParseImage(expression_);

            IsExpressionOutOfDate = false;
        }

        private void UpdateInfo()
        {
            IsInfoOutOfDate = true;

            if (expressionInfo_ != null)
            {
                if (image_ != null && image_.GetType() != WatchedImageTypeMap.GetImageType(expressionInfo_.Type))
                {
                    DisposeImage();
                }

                if (image_ == null)
                {
                    DebugLogAction("create image");
                    image_ = WatchedImageTypeMap.CreateInstance(expressionInfo_.Type, out matchedName_);
                }

                if (image_ != null)
                {
                    DebugLogAction("update image info");

                    List<string> newMatchedNames = new List<string>();
                    newMatchedNames.Add(matchedName_);

                    ExpressionHelper.InitializeImage(image_, expressionInfo_, newMatchedNames);

                    if (!matchedNames_.SequenceEqual(newMatchedNames))
                    {
                        // Evaluating expression for the first time or the types had changed.
                        matchedNames_ = newMatchedNames;
                    }

                    image_.ReloadInfo();
                }
            }

            IsInfoOutOfDate = false;
        }

        private void UpdatePixels()
        {
            ArePixelsOutOfDate = true;

            if (image_ != null && image_.HasValidInfo()
                && image_.GetInfo().HasPixels)
            {
                DebugLogAction("load pixels");
                image_.ReloadPixels();
            }

            ArePixelsOutOfDate = false;
        }

        private void UpdateContext()
        {
            DebugLogAction("update context");

            var oldInfo = expressionInfo_;

            // update expression without invalidating first, so that we save 
            // time if the expression is the same in the new context 
            UpdateExpression(false);

            if (oldInfo == null || !oldInfo.Equals(expressionInfo_))
                IsInfoOutOfDate = true;

            IsContextOutOfDate = false;
        }

        // detailLevel: 0 = expression, 1 = expression + info,
        // 2 = expression + info + pixels
        private void UpdateIfNecessary(int detailLevel = -1)
        {
            if (!IsActive || !IsInScope)
                return;

            if (IsContextOutOfDate)
                UpdateContext();

            if (IsExpressionOutOfDate && detailLevel > -1)
                UpdateExpression(true);

            if (!(IsExpanded || view_ != null))
                return;

            if (IsInfoOutOfDate && detailLevel > 0)
                UpdateInfo();

            if (ArePixelsOutOfDate && detailLevel > 1)
                UpdatePixels();
        }

        private string expression_ = "";
        public string Expression
        {
            get
            {
                return expression_;
            }
            set
            {
                SetProperty(ref expression_, value);
                MarkExpressionOutOfDate();
            }
        }

        public bool IsOperatorExpression
        {
            get
            {
                return ExpressionHelper.IsOperator(Expression);
            }
        }

        public bool IsValidExpression
        {
            get
            {
                UpdateIfNecessary(0);

                return expressionInfo_ != null;
            }
        }

        public string ExpressionDisplayType
        {
            get
            {
                UpdateIfNecessary(0);

                if (!IsValidExpression || expressionInfo_ == null)
                    return Microsoft.ImageWatch.Resources.InvalidText;

                return IsOperatorExpression ?
                    Microsoft.ImageWatch.Resources.OperatorText : expressionInfo_.Type;
            }
        }

        private WatchedImage image_ = null;
        private string matchedName_ = string.Empty; // The primary name of image_.
        private List<string> matchedNames_ = new List<string>(); // All names for a complex item.

        public WatchedImage Image
        {
            get
            {
                UpdateIfNecessary(1);

                return image_;
            }
        }

        public bool ImageHasValidInfo
        {
            get
            {
                UpdateIfNecessary(1);

                return image_ != null ? image_.HasValidInfo() : false;
            }
        }

        public bool ImageIsInitialized
        {
            get
            {
                UpdateIfNecessary(1);

                if (image_ == null || !image_.HasValidInfo())
                    return false;

                var info = image_.GetInfo();

                return info.IsInitialized;
            }
        }

        public bool ImageHasNativePixelValues
        {
            get
            {
                UpdateIfNecessary(1);

                if (image_ == null || !image_.HasValidInfo())
                    return false;

                var info = image_.GetInfo();

                return info.HasNativePixelValues;
            }
        }

        public bool ImageHasNativePixelAddress
        {
            get
            {
                UpdateIfNecessary(1);

                if (image_ == null || !image_.HasValidInfo())
                    return false;

                var info = image_.GetInfo();

                return info.HasNativePixelAddress;
            }
        }

        public string ImageSizeString
        {
            get
            {
                UpdateIfNecessary(1);

                if (image_ == null || !image_.HasValidInfo())
                    return Microsoft.ImageWatch.Resources.InvalidText;

                var info = image_.GetInfo();

                if (!ImageIsInitialized)
                    return Microsoft.ImageWatch.Resources.NoInitText;

                return string.Format("{0} x {1}", info.Width, info.Height);
            }
        }

        public string ImageFormatString
        {
            get
            {
                UpdateIfNecessary(1);

                if (image_ == null || !image_.HasValidInfo())
                    return Microsoft.ImageWatch.Resources.InvalidText;

                var info = image_.GetInfo();

                if (!ImageIsInitialized)
                    return Microsoft.ImageWatch.Resources.NoInitText;

                var str = string.Format("{0} x {1}", info.NumBands,
                    info.ElementFormat);

                if (!string.IsNullOrEmpty(info.PixelFormat))
                    str += string.Format(" [{0}]", info.PixelFormat);

                return str;
            }
        }

        private static string MakeBitmapInfoString(WatchListItem wli,
            InteropBitmap bmp)
        {
            if (wli == null || !wli.IsValidExpression)
                return Resources.InvalidTextWithoutBrackets;

            if (!wli.ImageHasValidInfo)
                return Resources.InvalidTextWithoutBrackets;

            var info = wli.Image.GetInfo();
            System.Diagnostics.Debug.Assert(info != null);
            if (info == null)
                return Resources.InvalidTextWithoutBrackets;

            if (info.HasNativePixelAddress && info.PixelAddress == 0)
                return Resources.NullText;

            if (!info.HasPixels)
                return Resources.EmptyText;

            // couldnt't render?
            if (bmp == null)
                return Resources.NotApplicableText;

            return null;
        }

        private WatchListItemView bitmap_ = null;

        private WatchListItemView thumb_ = null;
        public InteropBitmap Thumbnail
        {
            get
            {
#if DEBUG
                System.Diagnostics.Debug.WriteLine("getting Thumbnail '" +
                    Expression + "'");
#endif

                UpdateIfNecessary(2);

                if (image_ == null || !image_.HasPixelsLoaded())
                    return null;

                if (thumb_ == null)
                {
                    DebugLogAction("create thumbnail");
                    thumb_ = new WatchListItemView();
                }

                thumb_.Initialize(thumbnailWidth_, thumbnailHeight_,
                    bitmapBackground_, false);
                thumb_.NearScale = Math.Max(thumbnailWidth_, thumbnailHeight_);
                thumb_.SetImage(image_);
                thumb_.SetDefaultTransform();

                ApplyColormap(thumb_);

                return thumb_.Bitmap;
            }
        }

        public string ThumbnailInfoString
        {
            get
            {
                return MakeBitmapInfoString(this, Thumbnail);
            }
        }

        private int thumbnailWidth_;
        public int ThumbnailWidth
        {
            get
            {
                return thumbnailWidth_;
            }
            private set
            {
                SetProperty(ref thumbnailWidth_, value);
            }
        }

        private int thumbnailHeight_;
        public int ThumbnailHeight
        {
            get
            {
                return thumbnailHeight_;
            }
            private set
            {
                SetProperty(ref thumbnailHeight_, value);
            }
        }

        public InteropBitmap Bitmap
        {
            get
            {
                UpdateIfNecessary(2);

                if (image_ == null || !image_.HasPixelsLoaded())
                    return null;
                if (bitmap_ == null)
                {
                    bitmap_ = new WatchListItemView();
                }
                bitmap_.Initialize(image_.Rectangle.Width, Image.Rectangle.Height,
                    bitmapBackground_, false);
                bitmap_.SetImage(image_);
                ApplyColormap(bitmap_);
                return bitmap_.Bitmap;
            }
        }

        private Color bitmapBackground_;
        public Color BitmapBackground
        {
            get
            {
                return bitmapBackground_;
            }
            private set
            {
                SetProperty(ref bitmapBackground_, value);
            }
        }

        public void SetThumbnailProperties(int width, int height,
            Color backgroundColor)
        {
            thumbnailWidth_ = width;
            thumbnailHeight_ = height;
            bitmapBackground_ = backgroundColor;

            if (thumb_ != null)
            {
                thumb_.Initialize(thumbnailWidth_, thumbnailHeight_,
                    bitmapBackground_, false);
            }
        }

        public InteropBitmap View
        {
            get
            {
#if DEBUG
                System.Diagnostics.Debug.WriteLine("getting View '" +
                    Expression + "'");
#endif

                UpdateIfNecessary(2);

                if (image_ == null || !image_.HasPixelsLoaded())
                    return null;

                if (view_ == null)
                    return null;

                viewTransform_.ApplyAndSave(image_, view_);

                ApplyColormap(view_);

                return view_.Bitmap;
            }
        }

        public string ViewInfoString
        {
            get
            {
                return MakeBitmapInfoString(this, View);
            }
        }

        private WatchListItemViewTransform viewTransform_ =
            new WatchListItemViewTransform();

        private WatchListItemView view_ = null;

        public void ModifyViewTransformIfSizeMatches(Point center,
            double scale, Size imageSize)
        {
            if (ImageIsInitialized && Image.GetInfo().Width == imageSize.Width
                && Image.GetInfo().Height == imageSize.Height)
            {
                viewTransform_.Save(center, scale, imageSize);
            }
        }

        public void AttachView(WatchListItemView view)
        {
            if (view == null)
                return;

            view_ = view;
            view_.SizeChanged += ViewSizeChanged;

            NotifyAllPropertiesChanged();
        }

        public bool DetachView(ref Point center, ref double scale,
            ref Size imageSize)
        {
            if (view_ != null)
            {
                viewTransform_.ApplyAndSave(image_, view_);

                center = viewTransform_.Center;
                scale = viewTransform_.Scale;
                imageSize = viewTransform_.LastImageSize;

                view_.SizeChanged -= ViewSizeChanged;
                view_ = null;

                return true;
            }

            return false;
        }

        private void ViewSizeChanged(object sender, EventArgs e)
        {
            viewTransform_.ApplyAndSave(image_, view_);
            NotifyAllPropertiesChanged();
        }

        public void ZoomViewToFit()
        {
            if (view_ != null)
            {
                view_.SetDefaultTransform();
                viewTransform_.Save(image_, view_);
                NotifyAllPropertiesChanged();
            }
        }

        public void ZoomViewToOriginalSize(Point? targetCenter)
        {
            if (view_ != null)
            {
                if (!targetCenter.HasValue)
                {
                    view_.SetDefaultTransform();
                    view_.SetTransformScale(1.0);
                }
                else
                {
                    view_.ZoomInAt(targetCenter.Value,
                        1 / (view_.TransformScale + 1e-9));
                }

                viewTransform_.Save(image_, view_);
                NotifyAllPropertiesChanged();
            }
        }

        public void ZoomViewAtPosition(Point bitmapPos, double factor)
        {
            if (view_ != null)
            {
                view_.ZoomInAt(bitmapPos, factor);
                viewTransform_.Save(image_, view_);
                NotifyAllPropertiesChanged();
            }
        }

        public Point GetViewTransformCenterTarget()
        {
            return view_ != null ? view_.TransformCenterTarget : new Point();
        }

        public double GetViewTransformScale()
        {
            return view_ != null ? view_.TransformScale : 1.0;
        }

        public Vector SetViewTransformCenterTarget(Point centerTarget)
        {
            var correction = new Vector(0, 0);

            if (view_ != null)
            {
                correction = view_.SetTransformCenterTarget(centerTarget);
                viewTransform_.Save(image_, view_);
                NotifyAllPropertiesChanged();
            }

            return correction;
        }

        public bool IsViewTargetPointInsideImage(Point targetPos)
        {
            return View != null ? view_.IsTargetPointInsideSource(targetPos)
                : false;
        }

        public Point GetViewSourcePosFromTargetPos(Point targetXY)
        {
            return view_ != null ? view_.GetSourcePosFromTargetPos(targetXY) :
                new Point(0, 0);
        }

        private bool imageMinMaxOutOfDate = true;
        private bool autoScaleColormap_ = false;
        public void SetAutoScaleColormap(bool value)
        {
            if (!autoScaleColormap_ && value)
                imageMinMaxOutOfDate = true;

            autoScaleColormap_ = value;
        }

        private bool colormapJet_ = false;
        public void SetColormapJet(bool value)
        {
            colormapJet_ = value;
        }

        private bool fourChannelsIgnoreAlpha_ = false;
        public void SetFourChannelsIgnoreAlpha(bool value)
        {
            fourChannelsIgnoreAlpha_ = value;
        }

        private void ApplyColormap(WatchListItemView v)
        {
            if (v == null || image_ == null || image_.IsDisposed)
                return;

            var info = image_.GetInfo();

            if (autoScaleColormap_)// ?? && image_.GetInfo().NumBands == 1)
            {
                if (imageMinMaxOutOfDate)
                {
#if DEBUG
                    System.Diagnostics.Debug.WriteLine("++ computing min/max");
#endif
                    image_.ComputeMinMaxElementValue(-1);
                    imageMinMaxOutOfDate = false;
                }

                v.SetColormapDomain(image_.MinElementValue,
                    image_.MaxElementValue);
            }
            else if (info != null && info.HasCustomValueRange &&
                info.CustomValueRangeStart <= info.CustomValueRangeEnd)
            {
                v.SetColormapDomain(image_.GetInfo().CustomValueRangeStart,
                    image_.GetInfo().CustomValueRangeEnd);
            }
            else
            {
                v.SetDefaultColormapDomain();
            }

            v.SetColormapJet(colormapJet_);

            v.SetColormapFourChannelsIgnoreAlpha(fourChannelsIgnoreAlpha_);
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
                if (thumb_ != null)
                {
                    thumb_.Dispose();
                    thumb_ = null;
                }
                if (image_ != null)
                {
                    DisposeImage();
                }
                if (bitmap_ != null)
                {
                    bitmap_.Dispose();
                    bitmap_ = null;
                }
            }
        }

        private void DisposeImage()
        {
            image_.Dispose();
            image_ = null;
            matchedName_ = string.Empty;
            matchedNames_ = new List<string>();
        }
    }

    class WatchListItemViewTransform
    {
        public Point Center { get; private set; }
        public double Scale { get; private set; }
        public Size LastImageSize { get; private set; }

        private bool IsValid = false;

        private static Size GetImageSize(WatchedImage image)
        {
            return image != null && image.HasValidInfo() ?
                new Size(image.GetInfo().Width, image.GetInfo().Height)
                : Size.Empty;
        }

        public void Save(Point center, double scale, Size imageSize)
        {
            Center = center;
            Scale = scale;
            LastImageSize = imageSize;
            IsValid = true;
        }

        public void Save(WatchedImage image, WatchListItemView view)
        {
            IsValid = false;

            if (view != null && image != null && image.HasValidInfo())
            {
                Save(view.TransformCenter, view.TransformScale,
                    GetImageSize(image));
            }
        }

        public void ApplyAndSave(WatchedImage image, WatchListItemView view)
        {
            if (view != null)
            {
                view.SetImage(image);

                if (IsValid &&
                    image != null && image.HasValidInfo() &&
                    LastImageSize == GetImageSize(image))
                {
                    // note: center & scale need to be set atomically because
                    // of ClampTransform(), which otherwise over-corrects if
                    // the transform is only partially specified at the time
                    // ClampTransform() is called
                    view.SetTransformCenterAndScale(Center, Scale);
                }
                else
                {
                    view.SetDefaultTransform();
                }
            }

            Save(image, view);
        }
    }
}