using System;
using System.Windows;
using System.ComponentModel;
using System.Windows.Input;
using edlink_n8;
using System.Threading;
using System.IO.Compression;

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
				if (FilePath.ToLower().EndsWith(".zip"))
				{
					//ZipFile.ExtractToDirectory(FilePath, "FileCache/Unzips/");
					ZipArchive Zip = ZipFile.Open(FilePath, ZipArchiveMode.Read);
					var Blah = Zip.Entries;
				}
				LoadROM(FilePath);
			}
		}

		public static void LoadROM(string RomPath)
		{			
			Console.WriteLine("ROM loading...");

			try
			{
				TheEdio.EnableAsync(false);
				NesRom Rom = new NesRom(RomPath);
			
				Rom.print();

				if (Rom.Type == NesRom.ROM_TYPE_OS)
				{
					TheUsbio.loadOS(Rom, null);
				}
				else
				{
					TheUsbio.loadGame(Rom, null);									
				}

				Console.WriteLine();
				
				// Reconnect since the everdrive has to soft reboot to load a new ROM
				TheEdio.getConfig().print();
				Thread.Sleep(1000);
				TheUsbio.Connect();
				TheEdio.EnableAsync(true);
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
			}
		}
	}
}
