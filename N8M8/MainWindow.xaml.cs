using System;
using System.Windows;
using System.ComponentModel;
using System.Windows.Input;
using edlink_n8;
using System.Threading;

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

		private void BtnReboot_Click(object sender, RoutedEventArgs e)
		{
			TheUsbio.Reboot();			
		}

		private void Window_Drop(object sender, DragEventArgs e)
		{
			// If this is a file drop
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				// Grab the first string
				string FilePath = ((string[])e.Data.GetData(DataFormats.FileDrop))[0];
				LoadROM(FilePath);
			}
		}

		void LoadROM(string RomPath)
		{			
			Console.WriteLine("ROM loading...");

			NesRom Rom = new NesRom(RomPath);
			Rom.print();

			if (Rom.Type == NesRom.ROM_TYPE_OS)
			{
				TheUsbio.loadOS(Rom, null);
			}
			else
			{
				TheUsbio.loadGame(Rom, null);				
				TheEdio.Reinit();
			}

			Console.WriteLine();

			TheEdio.EnableAsync(false);
			TheEdio.getConfig().print();
			Thread.Sleep(1000);
			TheUsbio.Connect();
			TheEdio.EnableAsync(true);
		}
	}
}
