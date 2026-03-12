using EnvDTE;
using Microsoft.Research.NativeImage;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.Win32;
using System;
using System.ComponentModel.Design;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;

namespace Microsoft.ImageWatch
{
    /*[ClassInterface(ClassInterfaceType.AutoDual)]
    [CLSCompliant(false), ComVisible(true)]
    public class OptionPageGrid : DialogPage
    {
        [Category("Misc")]
        [DisplayName("Enable \"Add to Watch\" Links")]
        [Description("Shows a magnifying glass next to image objects in Visual Studio's Autos, Locals, and Watch window (and in the tooltip that shows up when hovering over a variable in the text editor while debugging. "
         + "\n\nNote: this can cause a flickering wait cursor and slow down debugging when many image objects are in scope.")]
        public bool EnableVisualizerLinks
        {

            get { return true; }
            set { }
        }

        static XNode CommentNode(XNode node)
        {
            if (node is XComment)
                return node;

            return new XComment(node.ToString());
        }

        static XNode UncommentNode(XNode node)
        {
            if (!(node is XComment))
                return node;

            return XDocument.Parse((node as XComment).Value).Root;
        }

        static bool EnableVisualizer(bool enable)
        {
            try
            {
                var loc = System.Reflection.Assembly.GetExecutingAssembly().Location;
                var path = Path.Combine(loc, "ImageWatchVisualizer.xml");

                var doc = XDocument.Load(path);
                var node = doc.Root.DescendantNodes().Skip(1).First();

                node.ReplaceWith(enable ? UncommentNode(node) : CommentNode(node));

                doc.Save(path);

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }
    }*/

    [Guid("A452AFEA-3DF6-46BB-9177-C0B08F318025")]
    public interface IImageWatchVisualizerService { }

    [ProvideService(typeof(IImageWatchVisualizerService), ServiceName = "ImageWatchVisualizerService")]
    /// <summary>
    /// This is the class that implements the package exposed by this assembly.
    ///
    /// The minimum requirement for a class to be considered a valid package for Visual Studio
    /// is to implement the IVsPackage interface and register itself with the shell.
    /// This package uses the helper classes defined inside the Managed Package Framework (MPF)
    /// to do it: it derives from the Package class that provides the implementation of the 
    /// IVsPackage interface and uses the registration attributes defined in the framework to 
    /// register itself and its components with the shell.
    /// </summary>
    [ProvideAutoLoad("ADFC4E63-0397-11D1-9F4E-00A0C911004F", PackageAutoLoadFlags.BackgroundLoad)]
    // This attribute tells the PkgDef creation utility (CreatePkgDef.exe) that this class is
    // a package.
    [PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
    // This attribute is used to register the information needed to show this package
    // in the Help/About dialog of Visual Studio.
    [InstalledProductRegistration("#110", "#112", "1.0", IconResourceID = 400)]
    // This attribute is needed to let the shell know that this package exposes some menus.
    [ProvideMenuResource("Menus.ctmenu", 1)]
    // This attribute registers a tool window exposed by this package.
    [ProvideToolWindow(typeof(MyToolWindow))]
    [ProvideService(typeof(IImageWatchAddWatchService), ServiceName = "ImageWatchAddWatchService")]
    [Guid(GuidList.guidImageWatchPkgString)]
    //[ProvideOptionPage(typeof(OptionPageGrid), "Image Watch", "Misc", 0, 0, true)]
    public sealed class ImageWatchPackage : AsyncPackage, IVsDebuggerEvents, IVsShellPropertyEvents
    {
        public static string InitError { get; private set; }

        /// <summary>
        /// Default constructor of the package.
        /// Inside this method you can place any initialization code that does not require 
        /// any Visual Studio service because at this point the package object is created but 
        /// not sited yet inside Visual Studio environment. The place to do all the other 
        /// initialization is the Initialize method.
        /// </summary>
        public ImageWatchPackage()
        {
            Package = this;
        }

        public static void InitializeTypes()
        {
            ImageWatchPackage.ClearLog();
            ImageWatchPackage.Log(DateTime.Now.ToLocalTime().ToString());

            WatchedImageTypeMap.Initialize(
                ImageWatchPackage.Config != null ?
                ImageWatchPackage.Config.EnableMicrosoftInternalTypes
                : false, true);
        }

        /// <summary>
        /// This function is called when the user clicks the menu item that shows the 
        /// tool window. See the Initialize method to see how the menu item is associated to 
        /// this function using the OleMenuCommandService service and the MenuCommand class.
        /// </summary>
        private void ShowToolWindow(object sender, EventArgs e)
        {
            // Get the instance number 0 of this tool window. This window is single instance so this instance
            // is actually the only one.
            // The last flag is set to true so that if the tool window does not exists it will be created.
            ToolWindowPane window = this.FindToolWindow(typeof(MyToolWindow), 0, true);
            if ((null == window) || (null == window.Frame))
            {
                throw new NotSupportedException(Resources.CanNotCreateWindow);
            }
            IVsWindowFrame windowFrame = (IVsWindowFrame)window.Frame;
            Microsoft.VisualStudio.ErrorHandler.ThrowOnFailure(windowFrame.Show());
        }

        uint initCookie;

        /////////////////////////////////////////////////////////////////////////////
        // Overridden Package Implementation
        #region Package Members

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initialization code that rely on services provided by VisualStudio.
        /// </summary>
        protected override async System.Threading.Tasks.Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
        {
#if DEBUG
            System.Diagnostics.Debug.WriteLine(string.Format(System.Globalization.CultureInfo.CurrentCulture, "Entering Initialize() of: {0}", this.ToString()));
#endif
            await base.InitializeAsync(cancellationToken, progress);

            // Add our command handlers for menu (commands must exist in the .vsct file)
            OleMenuCommandService mcs = await GetServiceAsync(typeof(IMenuCommandService)) as OleMenuCommandService;
            if (null != mcs)
            {
                // Create the command for the tool window
                CommandID toolwndCommandID = new CommandID(GuidList.guidImageWatchCmdSet, (int)PkgCmdIDList.cmdidImageWatch);
                MenuCommand menuToolWin = new MenuCommand(ShowToolWindow, toolwndCommandID);
                mcs.AddCommand(menuToolWin);               
            }

            IAsyncServiceContainer serviceContainer = (IAsyncServiceContainer)this;

            if (serviceContainer != null)
            {
                serviceContainer.AddService(typeof(IImageWatchVisualizerService), new AsyncServiceCreatorCallback(CreateImageWatchVisualizerService), true);
                serviceContainer.AddService(typeof(IImageWatchAddWatchService), new AsyncServiceCreatorCallback(CreateAddWatchService), true);
            }

            object dte = await GetServiceAsync(typeof(Microsoft.VisualStudio.Shell.Interop.SDTE));
            if (dte != null)
            {
                Config = new ImageWatchConfig();

                NativeImageHelpers.SetImageCacheMaxMemoryUsageAbsolute(
                    Config.MaxPixelMemoryUsageInMegabytes);
            }
            else // The IDE is not yet fully initialized
            {
                IVsShell shellService = await GetServiceAsync(typeof(SVsShell)) as IVsShell;

                if (shellService == null ||
                    shellService.AdviseShellPropertyChanges(this, out initCookie) != 0)
                {
                    InitError = "Initialization failed: shell service not found";
                }
            }

            await this.JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
            var debugger = await this.GetServiceAsync<SVsShellDebugger, IVsDebugger>();
            debugger.AdviseDebuggerEvents(this, out _);
        }
        #endregion

        private System.Threading.Tasks.Task<object> CreateImageWatchVisualizerService(IAsyncServiceContainer container, CancellationToken cancellationToken, Type serviceType)
        {
            return System.Threading.Tasks.Task.FromResult<object>(new ImageWatchVisualizerService());
        }

        private System.Threading.Tasks.Task<object> CreateAddWatchService(IAsyncServiceContainer container, CancellationToken cancellationToken, Type serviceType)
        {
            return System.Threading.Tasks.Task.FromResult<object>(AddWatchService);
        }

        public int OnModeChange(DBGMODE dbgmodeNew)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            return VSConstants.S_OK;
        }

        // see http://blogs.msdn.com/b/vsxteam/archive/2008/06/09/dr-ex-why-does-getservice-typeof-envdte-dte-return-null.aspx
        public int OnShellPropertyChange(int propid, object var)
        {
            if ((int)__VSSPROPID.VSSPROPID_Zombie == propid)
            {
                if ((bool)var == false)
                {
                    // finish initialization
                    Config = new ImageWatchConfig();

                    NativeImageHelpers.SetImageCacheMaxMemoryUsageAbsolute(
                        Config.MaxPixelMemoryUsageInMegabytes);
                    
                    // eventlistener no longer needed
                    IVsShell shellService = GetService(typeof(SVsShell)) as IVsShell;

                    if (shellService != null)
                        shellService.UnadviseShellPropertyChanges(initCookie);

                    initCookie = 0;
                }
            }

            return VSConstants.S_OK;

        }

        public static ImageWatchPackage Package { get; private set; }

        public static void ShowToolWindow()
        {
            if (Package != null)
                Package.ShowToolWindow(null, null);
        }

        static EnvDTE.DTE dte_;
        public static EnvDTE.DTE DTE
        {
            get
            {
                if (dte_ == null)
                {
                    try
                    {
                        dte_ = (EnvDTE.DTE)GetGlobalService(typeof(EnvDTE.DTE));
                    }
                    catch (Exception)
                    {
                    }
                }

                return dte_;
            }
        }

        public static ImageWatchConfig Config { get; private set; }

        public static ImageWatchAddWatchService AddWatchService = new ImageWatchAddWatchService();

        public static string UserPath
        {
            get
            {
                try
                {
                    var rr = DTE.RegistryRoot;
                    string v = (string)Registry.CurrentUser.OpenSubKey(rr)
                        .GetValue("VisualStudioLocation");

                    return Path.Combine(v, "Visualizers");
                }
                catch (Exception)
                {
                    return null;
                }
            }
        }

        public static string DefaultVisualizerPath
        {
            get
            {
                try
                {
                    var rr = DTE.RegistryRoot;
                    return Path.Combine((string) Registry.CurrentUser.OpenSubKey(rr + "_Config")
                        .GetValue("ShellFolder"), @"Common7\Packages\Debugger\Visualizers");
                }
                catch (Exception)
                {
                    return null;
                }
            }
        }

        static string LogPath
        {
            get
            {
                return Path.Combine(UserPath, "ImageWatch.log");
            }
        }

        public static void ClearLog()
        {
            try
            {
                if (File.Exists(LogPath))
                    File.Delete(LogPath);
            }
            catch (Exception)
            {
            }
        }

        public static void Log(string msg)
        {
            try
            {
                File.AppendAllText(LogPath, "+ " + msg + "\n");
            }
            catch (Exception)
            {
            }
        }
    }
}
