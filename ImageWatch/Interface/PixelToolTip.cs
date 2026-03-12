using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.OLE.Interop;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace Microsoft.ImageWatch.Interface
{
    public class PixelToolTip : Popup
    {
        [DllImport("user32.dll")]
        private static extern bool MoveWindow(IntPtr hwnd, int x, int y,
            int w, int h, bool bRepaint);

        [DllImport("user32.dll")]
        private static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);

        private IntPtr? GetHwnd()
        {
            if (Child == null)
                return null;

            var hs = (PresentationSource.FromVisual(Child)) as HwndSource;
            if (hs == null)
                return null;

            return hs.Handle;
        }

        private bool isMoving_ = false;
        public void MoveTo(Point pos)
        {
            if (isMoving_)
                return;

            isMoving_ = true;

            try
            {
                var hwnd = GetHwnd();
                if (!hwnd.HasValue)
                    return;

                RECT rect;
                if (!GetWindowRect(hwnd.Value, out rect))
                    return;

                MoveWindow(hwnd.Value, (int)pos.X, (int)pos.Y, (int)Width,
                    (int)Height, true);
            }
            finally
            {
                isMoving_ = false;
            }
        }

        public Point? Position
        {
            get
            {
                var hwnd = GetHwnd();
                if (!hwnd.HasValue)
                    return null;

                RECT rect;
                if (!GetWindowRect(hwnd.Value, out rect))
                    return null;

                return new Point(rect.left, rect.top);
            }
        }
    }
}
