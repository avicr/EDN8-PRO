using System;
using System.Diagnostics;
using edlink_n8;
using System.Windows.Controls;
using System.Collections.Specialized;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows;
using System.ComponentModel;

namespace N8M8.Controls
{
	// List view code adapted from: https://gargmanoj.wordpress.com/2009/03/27/wpf-101-a-simple-windows-explorer-like-directory-browser/
	public class MyListItem
	{
		public string Icon { get; set; }
		public string Text { get; set; }
	}

	/// <summary>
	/// Interaction logic for EDFileBrowser.xaml
	/// </summary>
	public partial class EverdriveMate : UserControl
	{		
		protected Edio TheEdio;
		protected Usbio TheUsbio;
		protected string CurrentFolderPath = "";

		// Local relative PC directory to copy everdive files to
		public const string CacheDirectory = "./FileCache/";

		public EverdriveMate()
		{			
			InitializeComponent();

			if (!DesignerProperties.GetIsInDesignMode(this))
			{
				try
				{
					TheEdio = new Edio();
					TheUsbio = new Usbio(TheEdio);

					if (TheEdio != null)
					{
						Browse("/", false);
					}

					SetupDelegates();					

				}
				catch (Exception e)
				{
					MessageBox.Show("Error initializing everdrive: " + e.Message);
				}
			}
		}

		public void SetupDelegates()
		{
			TheEdio.OnGameStarted += GameStarted;
			TheEdio.OnGameStopped += GameStopped;
			TheEdio.OnSaveStateLoaded += SaveStateLoaded;
			TheEdio.OnSaveStateSaved += SaveStateSaved;
		}
		public void GameStarted(Object Sender, string FilePath)
		{
			Debug.WriteLine("Game started: " + FilePath);
		}

		public void GameStopped(Object Sender, string FilePath)
		{
			Debug.WriteLine("Game Stopped: " + FilePath);
		}

		public void SaveStateLoaded(Object Sender, string FilePath)
		{
			Debug.WriteLine("Save State Loaded: " + FilePath);
		}

		public void SaveStateSaved(Object Sender, string FilePath)
		{
			Debug.WriteLine("Save State Saved: " + FilePath);
		}

		public void SetDirectoryInfo(EDFileInfo[] FileInfos)
		{			
			LstDirContents.Items.Clear();
			
			foreach (EDFileInfo CurFile in FileInfos)
			{
				eItemType FileType = 0;
				// If this is a directory
				if ((CurFile.attrib & 0x10) > 0)
				{
					FileType = eItemType.Directory;
				}
				else if (CurFile.name.EndsWith(".sav", StringComparison.OrdinalIgnoreCase))
				{
					FileType = eItemType.SaveStateFile;
				}
				else if (CurFile.name.EndsWith(".nes", StringComparison.OrdinalIgnoreCase))
				{
					FileType = eItemType.NesRomFile;
				}

				LstDirContents.Items.Add(new DirInfo { Path = CurrentFolderPath + "/", Name = CurFile.name, ItemType = FileType });
			}

			if (LstDirContents.Items.Count > 0)
			{
				LstDirContents.ScrollIntoView(LstDirContents.Items[0]);
			}
		}

		public void Browse(string NewPath, bool bIsRelative = false)
		{
			if (TheEdio != null)
			{
				// Prepend the directory deliminator
				if (!CurrentFolderPath.EndsWith('/'))
				{
					CurrentFolderPath += "/";
				}

				if (bIsRelative)
				{
					CurrentFolderPath += NewPath;
				}
				else
				{
					CurrentFolderPath = NewPath;
				}

				EDFileInfo[] Files = TheEdio.ChangeDirectory(CurrentFolderPath);
				SetDirectoryInfo(Files);

				// Add the root slash
				if (CurrentFolderPath == "")
				{
					CurrentFolderPath = "/";
				}
				TxtCurrentPath.Text = CurrentFolderPath;
			}
		}

		private void LstDirContents_MouseDoubleClick(object sender, MouseButtonEventArgs e)
		{
			
			if (LstDirContents.SelectedValue != null)
			{
				DirInfo SelectedDir = LstDirContents.SelectedValue as DirInfo;

				// If this is a directory, browse to it
				if (SelectedDir != null && SelectedDir.ItemType == eItemType.Directory)
				{
					Browse(SelectedDir.Name, true);
				}
			}
		}

		private void BtnUp_Click(object sender, RoutedEventArgs e)
		{
			GoBack();
		}

		private void Copy_Click(object sender, RoutedEventArgs e)
		{
			CopySelectedItemsToClipboard();			
		}

		private void Delete_Click(object sender, RoutedEventArgs e)
		{
			if (MessageBox.Show("Are you sure you want to delete the selected items?", "Delete", MessageBoxButton.YesNo) == MessageBoxResult.Yes)
			{
				try
				{
					DeleteSelectedItemsFromEverdrive();
				}
				catch (Exception ex)
				{
					if (ex.Source == "7")
					{
						MessageBox.Show("Cannot delete non-empty directory.");
					}
				}
			}
		}

		private void Paste_Click(object sender, RoutedEventArgs e)
		{
			CopyClipboardToEverdrive();
		}

		protected void CopySelectedItemsToClipboard()
		{
			StringCollection FilesToCopy = new StringCollection();

			foreach (DirInfo SelectedDir in LstDirContents.SelectedItems)
			{
				// Construct the full cache path
				bool bIsFolder = SelectedDir.ItemType == eItemType.Directory;
				string FileDestinationPath = System.IO.Path.GetFullPath(CacheDirectory + SelectedDir.Path);

				if (bIsFolder)
				{
					// Don't need the SelectedDir.Path if we're copying a folder
					FileDestinationPath = System.IO.Path.GetFullPath(CacheDirectory);
				}
				string FileSourcepath = "sd:" + SelectedDir.Path + SelectedDir.Name;


				// Copy the everdrive file to the cache path
				TheUsbio.copyFile(FileSourcepath, FileDestinationPath, bIsFolder);

				// Put the file path into the clipboard				
				FilesToCopy.Add(System.IO.Path.GetFullPath(CacheDirectory + SelectedDir.Path) + SelectedDir.Name);
			}

			if (FilesToCopy.Count > 0)
			{
				Clipboard.SetFileDropList(FilesToCopy);
			}
		}

		protected void DeleteSelectedItemsFromEverdrive()
		{
			StringCollection FilesToCopy = new StringCollection();

			foreach (DirInfo SelectedDir in LstDirContents.SelectedItems)
			{
				// Construct the full cache path
				bool bIsFolder = SelectedDir.ItemType == eItemType.Directory;				

				if (bIsFolder)
				{					
				}
				string FileSourcepath = SelectedDir.Path + SelectedDir.Name;

				// Refresh the view
				TheUsbio.DeleteFile(FileSourcepath);				
			}

			Browse(CurrentFolderPath, false);			
		}

		protected void CopyClipboardToEverdrive()
		{
			StringCollection FileDropList = Clipboard.GetFileDropList();

			foreach (string FilePath in FileDropList)
			{
				TheUsbio.copyFile(FilePath, "sd:" + CurrentFolderPath + "/" + System.IO.Path.GetFileName(FilePath));
			}

			// Refresh the folder view
			Browse(CurrentFolderPath, false);
		}

		private void GoBack()
		{
			int LastDelimiter = CurrentFolderPath.LastIndexOf('/');
			
			if (LastDelimiter > -1)
			{
				Browse(CurrentFolderPath.Substring(0, LastDelimiter), false);
			}
		}

		private void LstDirContents_MouseUp(object sender, MouseButtonEventArgs e)
		{
			if (e.ChangedButton == MouseButton.XButton1)
			{
				GoBack();
			}
		}

		private void LstDirContents_ContextMenuOpening(object sender, ContextMenuEventArgs e)
		{			
			// Start the context menu from scratch
			FrameworkElement fe = e.Source as FrameworkElement;
			ContextMenu cm = fe.ContextMenu;
			cm.Items.Clear();

			DirInfo SelectedDir = LstDirContents.SelectedValue as DirInfo;

			// If something is selected, add the Copy menu item
			if (SelectedDir != null)
			{				
				MenuItem NewMenu = new MenuItem();
				NewMenu.Header = "Copy";
				NewMenu.Click += Copy_Click;
				fe.ContextMenu.Items.Add(NewMenu);

				NewMenu = new MenuItem();
				NewMenu.Header = "Delete";
				NewMenu.Click += Delete_Click;
				fe.ContextMenu.Items.Add(NewMenu);
			}

			StringCollection FileDropList = Clipboard.GetFileDropList();
			if (FileDropList.Count > 0)
			{				
				MenuItem NewMenu = new MenuItem();
				NewMenu.Header = "Paste";
				NewMenu.Click += Paste_Click;
				fe.ContextMenu.Items.Add(NewMenu);
			}


		}

		private void UserControl_Unloaded(object sender, RoutedEventArgs e)
		{
			TheUsbio.Disconnect();
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			Window OwningWindow = Window.GetWindow(this);
			OwningWindow.Closing += OnWindowClosing;
		}

		void OnWindowClosing(object sender, global::System.ComponentModel.CancelEventArgs e)
		{
			TheUsbio.Disconnect();
		}
	}
	
}