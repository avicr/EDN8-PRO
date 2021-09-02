﻿using System;
using System.IO.Compression;
using System.Diagnostics;
using edlink_n8;
using System.Windows.Controls;
using System.Windows;
using System.Windows.Threading;
using System.ComponentModel;
using System.IO;
using System.Windows.Shapes;

namespace N8M8.Controls
{
	/// <summary>
	/// Interaction logic for EDInfo.xaml
	/// </summary>
	public partial class EDInfo : UserControl
	{
		public EDInfo()
		{
			InitializeComponent();
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			if (!DesignerProperties.GetIsInDesignMode(this))
			{
				SetupDelegates();
			}
		}

		public void SetupDelegates()
		{
			MainWindow.TheEdio.OnGameStarted += GameStarted;
			MainWindow.TheEdio.OnGameStopped += GameStopped;
			MainWindow.TheEdio.OnSaveStateLoaded += SaveStateLoaded;
			MainWindow.TheEdio.OnSaveStateSaved += SaveStateSaved;
			MainWindow.TheEdio.OnSaveStateSlotChanged += SaveStateSlotChanged;
		}

		public void GameStarted(Object Sender, string FilePath)
		{			
			if (!Dispatcher.CheckAccess())
			{
				Dispatcher.BeginInvoke(
					DispatcherPriority.Background,
					new Action(() => GameStarted(Sender, FilePath)));
			}
			else
			{
				LblGame.Content = System.IO.Path.GetFileName(FilePath);

				// Assume default save state folder/slot on game start
				LblSaveFolder.Content = "DEFAULT";
				LblSaveSlot.Content = "0";
				//TxtLog.AppendText(("Game started: " + FilePath + "\n"));
			}
			
		}

		public void GameStopped(Object Sender, string FilePath)
		{
			if (!Dispatcher.CheckAccess())
			{
				Dispatcher.BeginInvoke(
					DispatcherPriority.Background,
					new Action(() => GameStopped(Sender, FilePath)));
			}
			else
			{
				LblGame.Content = "None";
				LblSaveFolder.Content = "";
				LblSaveSlot.Content = "";
				//TxtLog.AppendText(("Game started: " + FilePath + "\n"));
			}
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

		public void SaveStateSlotChanged(Object Sender, Edio.SaveStateSlotChangeEventArgs Args)
		{
			if (!Dispatcher.CheckAccess())
			{				
				Dispatcher.BeginInvoke(
					DispatcherPriority.Background,
					new Action(() => SaveStateSlotChanged(Sender, Args)));
			}
			else
			{				
				LblSaveFolder.Content = Args.FolderName;
				LblSaveSlot.Content = Args.SlotNumber.ToString(Name);
				//TxtLog.AppendText(("Game started: " + FilePath + "\n"));
			}
			Debug.WriteLine("Save State Saved: " + Args.FolderName + " " + Args.SlotNumber.ToString());
		}

		private void MainPanel_PreviewGiveFeedback(object sender, GiveFeedbackEventArgs e)
		{
			MainPanel.Background.Opacity = 0.5;
		}

		private void MainPanel_PreviewDragEnter(object sender, DragEventArgs e)
		{
			Highlight.Visibility = Visibility.Visible;
		}

		private void MainPanel_PreviewDragLeave(object sender, DragEventArgs e)
		{
			Highlight.Visibility = Visibility.Hidden;
		}

		private void MainPanel_PreviewDragOver(object sender, DragEventArgs e)
		{
			
		}

		private void MainPanel_PreviewDrop(object sender, DragEventArgs e)
		{
			Highlight.Visibility = Visibility.Hidden;
			// If this is a file drop
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				// Grab the first string
				string FilePath = ((string[])e.Data.GetData(DataFormats.FileDrop))[0];
				
				// Unzip the ROM
				if (FilePath.ToLower().EndsWith(".zip"))
				{
					//ZipFile.ExtractToDirectory(FilePath, "FileCache/Unzips/");
					ZipArchive Zip = ZipFile.Open(FilePath, ZipArchiveMode.Read);
					
					foreach (ZipArchiveEntry Entry in Zip.Entries)
					{
						if (Entry.Name.ToLower().EndsWith(".nes"))
						{
							FilePath = Directory.GetCurrentDirectory() + "/FileCache/" + Entry.Name;
							if (File.Exists(FilePath))
							{
								File.Delete(FilePath);
							}

							Entry.ExtractToFile(FilePath);
						}
					}
					

				}

				MainWindow.LoadROM(FilePath);
			}
		}		
	}
}
