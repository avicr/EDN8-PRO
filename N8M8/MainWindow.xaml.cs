using System;
using System.Windows;
using System.ComponentModel;
using System.Windows.Input;
using edlink_n8;
using N8M8.Controls;

namespace N8M8
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		// Here we go putting important things in the main window again (probably should use a singleton)
		static public Edio TheEdio;
		static public Usbio TheUsbio;

		public MainWindow()
		{			
			InitializeComponent();

			if (!DesignerProperties.GetIsInDesignMode(this))
			{
				try
				{
					TheEdio = new Edio();
					TheUsbio = new Usbio(TheEdio);
				}
				catch (Exception e)
				{
					MessageBox.Show("Error initializing everdrive: " + e.Message);
				}
			}
		}

		private void FileBrowser_MouseDoubleClick(object sender, MouseButtonEventArgs e)
		{

		}

		private void Grid_Unloaded(object sender, RoutedEventArgs e)
		{

		}
	}
}
