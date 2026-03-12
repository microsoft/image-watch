using System;
using System.Linq;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Windows;
using System.Windows.Input;
using System.Windows.Forms;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Shell;

using Microsoft.ImageWatch.Interface;

namespace Microsoft.ImageWatch
{
    /// <summary>
    /// This class implements the tool window exposed by this package and hosts a user control.
    ///
    /// In Visual Studio tool windows are composed of a frame (implemented by the shell) and a pane, 
    /// usually implemented by the package implementer.
    ///
    /// This class derives from the ToolWindowPane class provided from the MPF in order to use its 
    /// implementation of the IVsUIElementPane interface.
    /// </summary>
    [Guid("3abea8e9-72f5-4294-b206-20a0eb527cf2")]
    public class MyToolWindow : ToolWindowPane
    {
        public static object FindResource(string name)
        {
            var fe = TheMainControl as FrameworkElement;
            if (fe == null)
                return null;

            return fe.FindResource(name);
        }

        private static MainControl TheMainControl { get; set; }
        public static Controller TheController { get; set; }

        /// <summary>
        /// Standard constructor for the tool window.
        /// </summary>
        public MyToolWindow() :
            base(null)
        {
            // Set the window title reading it from the resources.
            this.Caption = Resources.ToolWindowTitle;
            // Set the image that will appear on the tab of the window frame
            // when docked with an other window
            // The resource ID correspond to the one defined in the resx file
            // while the Index is the offset in the bitmap strip. Each image in
            // the strip being 16x16.
            this.BitmapResourceID = 301;
            this.BitmapIndex = 3;

            TheController = new Controller();

            if (!string.IsNullOrEmpty(ImageWatchPackage.InitError))
                TheController.ErrorState = ImageWatchPackage.InitError;

            // This is the user control hosted by the tool window; Note that, even if this class implements IDisposable,
            // we are not calling Dispose on this object. This is because ToolWindowPane calls Dispose on 
            // the object returned by the Content property.
            TheMainControl = new MainControl();
            TheMainControl.DataContext = TheController;
            base.Content = TheMainControl;

            TheController.FirstTimeEnterBreakMode();            
        }

        [DllImport("user32.dll")]
        static extern bool TranslateMessage([In] ref Message msg);

        [DllImport("user32.dll")]
        static extern IntPtr DispatchMessage([In] ref Message msg);

        protected override bool PreProcessMessage(ref Message msg)
        {
            if (msg.Msg == 0x100) // keydown?
            {
                var key = msg.WParam.ToInt32();

                if (TheMainControl != null)
                    TheMainControl.ParentKeyDown(key);

                if (key == 0x1b) // escape?
                {
                    if (System.Windows.Input.Keyboard.FocusedElement is
                        System.Windows.Controls.TextBox)
                    {
                        TranslateMessage(ref msg);
                        DispatchMessage(ref msg);
                        return true;
                    }
                }
            }
            else if (msg.Msg == 0x0101)
            {
                var key = msg.WParam.ToInt32();

                if (TheMainControl != null)
                    TheMainControl.ParentKeyUp(key);
            }

            return false;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (TheController != null)
                    TheController.Dispose();
            }

            base.Dispose(disposing);
        }
    }
}
