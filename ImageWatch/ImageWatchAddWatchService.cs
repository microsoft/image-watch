using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace Microsoft.ImageWatch
{
    [Guid("2CE087F4-6AB3-4BA7-AA66-1069C2F42AE3")]
    public interface IImageWatchAddWatchService 
    {
        void AddWatch(string expression);        
    }
    
    public class ImageWatchAddWatchEventArgs : EventArgs
    {
        public string Expression { get; set; }
    }
        
    public class ImageWatchAddWatchService : IImageWatchAddWatchService
    {
        public event EventHandler Add;

        private void OnAdd(string expression)
        {
            if (Add != null)
            {
                Add(this, new ImageWatchAddWatchEventArgs()
                {
                    Expression = expression
                });
            }
        }

        public void AddWatch(string expression)
        {
            ImageWatchPackage.ShowToolWindow();
            OnAdd(expression);
        }
    }
}
