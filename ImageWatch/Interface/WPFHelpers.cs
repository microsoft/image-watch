using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Data;
using System.Windows.Input;

namespace Microsoft.ImageWatch.Interface
{
    public static class WPFHelpers
    {
        public static ChildType FindVisualChild<ChildType>(DependencyObject o)
            where ChildType : DependencyObject
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(o); ++i)
            {
                DependencyObject child = VisualTreeHelper.GetChild(o, i);

                if (child != null && child is ChildType)
                {
                    return child as ChildType;
                }
                else
                {
                    ChildType childOfChild =
                        FindVisualChild<ChildType>(child);

                    if (childOfChild != null)
                        return childOfChild;
                }
            }

            return null;
        }

        public static bool ValidateConverterInput(object value)
        {
            return ValidateConverterInput(new object[] { value });
        }

        public static bool ValidateConverterInput(object[] values)
        {
            if (values.Any(v => v == DependencyProperty.UnsetValue))
            {
#if _DEBUG
                System.Diagnostics.Debug.WriteLine("Warning: " +
                    string.Join(" ", values));
#endif
                return false;
            }

            return true;
        }

        public static bool CtrlPressed()
        {
            return (Keyboard.IsKeyDown(Key.LeftCtrl) ||
                Keyboard.IsKeyDown(Key.RightCtrl));
        }

        public static bool ShiftPressed()
        {
            return (Keyboard.IsKeyDown(Key.LeftShift) ||
                Keyboard.IsKeyDown(Key.RightShift));
        }
    }

    public class NegatingConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return !(bool)value;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            return !(bool)value;
        }
    }

    public class NullToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return value != null && value != DependencyProperty.UnsetValue;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class IntToDoubleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return (double)(int)value;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class IntToGridLengthConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return new GridLength((int)value);
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class VisibilityToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return (Visibility)value == Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NegatingBooleanToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            return (bool)value ? Visibility.Collapsed : Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NullOrEmptyToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            var nullVisibility = parameter != null && (bool)parameter ?
                Visibility.Hidden : Visibility.Collapsed;

            return string.IsNullOrEmpty((string)value) ? nullVisibility :
                Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NegatingNullOrEmptyToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            var notNullVisibility = parameter != null && (bool)parameter ?
                Visibility.Hidden : Visibility.Collapsed;

            return !string.IsNullOrEmpty((string)value) ? notNullVisibility :
                Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    
    public class AndConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType,
            object parameter, System.Globalization.CultureInfo culture)
        {
            if (values == null)
                return false;

            foreach (var v in values)
            {
                if (v == DependencyProperty.UnsetValue)
                    return false;

                if (v == null)
                    return false;

                if (v is Boolean && !(bool)v)
                    return false;
            }

            return true;
        }

        public object[] ConvertBack(object value, Type[] targetTypes,
            object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
