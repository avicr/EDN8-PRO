// Code adapted (ok, copied) from: https://gargmanoj.wordpress.com/2009/03/27/wpf-101-a-simple-windows-explorer-like-directory-browser/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Collections;
namespace N8M8.Controls
{
	/// <summary>
	/// Enum to hold the Types of different file 
	/// </summary>
	public enum ObjectType
	{
		MyComputer = 0,
		DiskDrive = 1,
		Directory = 2,
		File = 3
	}
	public enum eItemType
	{
		GenericFile,
		Directory,
		SaveStateFile,
		NesRomFile,
	}

	/// <summary>
	/// Class for containing the information about a Directory/File
	/// </summary>
	public class DirInfo : DependencyObject
	{		

		#region // Public Properties
		public string Name { get; set; }
		public string Path { get; set; }
		public string Root { get; set; }
		public string Size { get; set; }
		public string Ext { get; set; }
		public eItemType ItemType { get; set; }
		#endregion
		#region // Dependency Properties
		public static readonly DependencyProperty propertyChilds = DependencyProperty.Register("Childs", typeof(IList<DirInfo>), typeof(DirInfo));
		public IList<DirInfo> SubDirectories
		{
			get { return (IList<DirInfo>)GetValue(propertyChilds); }
			set { SetValue(propertyChilds, value); }
		}
		public static readonly DependencyProperty propertyIsExpanded = DependencyProperty.Register("IsExpanded", typeof(bool), typeof(DirInfo));
		public bool IsExpanded
		{
			get { return (bool)GetValue(propertyIsExpanded); }
			set { SetValue(propertyIsExpanded, value); }
		}
		public static readonly DependencyProperty propertyIsSelected = DependencyProperty.Register("IsSelected", typeof(bool), typeof(DirInfo));
		public bool IsSelected
		{
			get { return (bool)GetValue(propertyIsSelected); }
			set { SetValue(propertyIsSelected, value); }
		}
		#endregion
		#region // .ctor(s)
		public DirInfo()
		{
			SubDirectories = new List<DirInfo>();
			SubDirectories.Add(new DirInfo("TempDir"));
		}
		public DirInfo(string directoryName)
		{
			Name = directoryName;
		}
		public DirInfo(DirectoryInfo dir)
			: this()
		{
			Name = dir.Name;
			Root = dir.Root.Name;
			Path = dir.FullName;
			ItemType = (eItemType)ObjectType.Directory;
		}
		public DirInfo(FileInfo fileobj)
		{
			Name = fileobj.Name;
			Path = fileobj.FullName;
			ItemType = (eItemType)ObjectType.File;
			Size = (fileobj.Length / 1024).ToString() + " KB";
			Ext = fileobj.Extension + " File";
		}
		public DirInfo(DriveInfo driveobj)
			: this()
		{
			if (driveobj.Name.EndsWith(@"\"))
				Name = driveobj.Name.Substring(0, driveobj.Name.Length - 1);
			else
				Name = driveobj.Name;
			Path = driveobj.Name;
			ItemType = (eItemType)ObjectType.DiskDrive;
		}
		#endregion
	}
}
