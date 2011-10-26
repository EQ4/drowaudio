/*
 *  dRowAudio_ColumnFileBrowser.cpp
 *  dRowAudio
 *
 *  Created by David Rowland on 27/03/2009.
 *  Copyright 2009 dRowAudio. All rights reserved.
 *
 */

BEGIN_JUCE_NAMESPACE

//==================================================================================
class BrowserColumn : public BasicFileBrowser,
                      public DragAndDropContainer,
                      public ChangeBroadcaster
{
public:
    //==================================================================================
	BrowserColumn (WildcardFileFilter* filesToDisplay_, int columnNumber_)
        : BasicFileBrowser (BasicFileBrowser::openMode
                            + BasicFileBrowser::canSelectFiles
                            + BasicFileBrowser::canSelectDirectories
                            + BasicFileBrowser::canSelectMultipleItems,
                            File::getSpecialLocation (File::userMusicDirectory),
                            filesToDisplay_),
          columnNumber (columnNumber_),
          active (false),
          fileDragEnabled (false)
	{
	}
	
	~BrowserColumn()
	{
	}
    
	void mouseDrag (const MouseEvent& e)
	{
		if (fileDragEnabled)
		{
			StringArray files;
            
            for (int i = 0; i < getNumSelectedFiles(); i++) 
            {
                File file (getSelectedFile (i));
                
                if (file.existsAsFile()) 
                    files.add (file.getFullPathName());
            }
            
            fileDragEnabled = false;
            performExternalDragDropOfFiles (files, false);
		}
	}
	
	void mouseUp (const MouseEvent& e)
	{
		fileDragEnabled = false;
	}
	
	void selectionChanged()
	{
        BasicFileBrowser::selectionChanged();
        
        sendChangeMessage();
	}
    
	void fileClicked (const File& file, const MouseEvent& e)
	{
		fileDragEnabled = true;
	}
	
	void fileDoubleClicked (const File& f) {}
    
private:
    //==================================================================================
	int columnNumber;
	bool active, fileDragEnabled;
	
	friend class ColumnFileBrowserContents;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserColumn);
};


//==================================================================================
class ColumnFileBrowserContents	: public Component,
                                  public FileBrowserListener,
                                  public ChangeListener,
                                  public ComponentListener
{
public:
    //==================================================================================
	ColumnFileBrowserContents (WildcardFileFilter* filesToDisplay_, Viewport* parentViewport);
	
	~ColumnFileBrowserContents();
	
	void resized();
	void selectionChanged () {}
	void fileClicked (const File& file, const MouseEvent& e) {}
	void fileDoubleClicked (const File& file) {}
	void browserRootChanged (const File& newRoot) {}
	void selectedFileChanged (const File& file);
	bool addColumn (const File& rootDirectory);
	void removeColumn (int numColumns = 1);
	void changeListenerCallback (ChangeBroadcaster* changedComponent);
	void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized);
	bool keyPressed (const KeyPress& key);
    
private:
    //==================================================================================
	WildcardFileFilter* filesToDisplay;
	Viewport* viewport;
	OwnedArray <BrowserColumn> columns;
	
	int activeColumn;
    
    ScopedPointer<ColumnFileBrowserLookAndFeel> activeLookAndFeel;  
    ScopedPointer<ColumnFileBrowserLookAndFeel> inactiveLookAndFeel;  
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColumnFileBrowserContents);
};

//==================================================================================
ColumnFileBrowserContents::ColumnFileBrowserContents (WildcardFileFilter* filesToDisplay_, Viewport* parentViewport)
    : filesToDisplay (filesToDisplay_),
	  viewport (parentViewport)
{
    activeLookAndFeel = new ColumnFileBrowserLookAndFeel();
    activeLookAndFeel->setColour (DirectoryContentsDisplayComponent::highlightColourId, 
                                  Colours::darkorange);
    inactiveLookAndFeel = new ColumnFileBrowserLookAndFeel();
    
	columns.add (new BrowserColumn (filesToDisplay_, 0));
	addAndMakeVisible (columns[0]);
	columns[0]->setSize (300, 50);
	columns[0]->columnNumber = columns.size() - 1;
	columns[0]->addListener (this);
	columns[0]->addChangeListener (this);
	columns[0]->addComponentListener (this);
    columns[0]->setLookAndFeel (activeLookAndFeel);
    
	activeColumn = 0;
}

ColumnFileBrowserContents::~ColumnFileBrowserContents()
{
}

void ColumnFileBrowserContents::resized()
{
	int width = 0;
	const int height = getHeight();
	
	for (int i = 0; i < columns.size(); i++)
	{
		columns[i]->setBounds (width, 0, columns[i]->getWidth(), height);
		width += columns[i]->getWidth();
	}
	
	setSize (width, height);
}

void ColumnFileBrowserContents::selectedFileChanged (const File& file)
{
    // if last column clicked add new column
	if (columns[activeColumn] == columns.getLast())
	{
		addColumn (file);
	}
	else        // otherwise remove uneeded columns and change last
	{
		for (int i = 0; i < columns.size(); i++)
		{
			if (columns[activeColumn] == columns[i])
            {
                const int numColumnsToRemove = columns.size() - i - (file.isDirectory() ? 1 : 0);
				removeColumn (numColumnsToRemove);
				
				if (file.isDirectory())
					columns.getLast()->setRoot (file);
			}
		}
		
		resized();
	}
	
	// stick to edges of viewport
	if (getWidth() < viewport->getWidth())
    {
		viewport->setViewPosition (0, 0);
    }
	else if (file.exists() || (getRight() < viewport->getRight()))
	{
		const int ammountToSubtract = viewport->getRight() - getRight();
		viewport->setViewPosition (viewport->getViewPositionX() - ammountToSubtract, 0);
	}
}

bool ColumnFileBrowserContents::addColumn (const File& rootDirectory)
{
	if (rootDirectory.isDirectory() && rootDirectory.exists())
	{
        const int startingWidth = columns.getLast()->getWidth();
        
		columns.add (new BrowserColumn (filesToDisplay, columns.size()));
		addAndMakeVisible (columns.getLast());
        columns.getLast()->setLookAndFeel (inactiveLookAndFeel);
		columns.getLast()->setRoot (rootDirectory);
		columns.getLast()->setSize (startingWidth, 50);
		columns.getLast()->addListener (this);
		columns.getLast()->addChangeListener (this);
		columns.getLast()->addComponentListener (this);
		
		resized();
		
		return true;
	}
	
	return false;
}

void ColumnFileBrowserContents::removeColumn (int numColumns)
{
	for (int i = numColumns; i <= 0; i--) 
    {
		columns[i]->removeListener (this);
		columns[i]->removeChangeListener (this);
		columns[i]->removeComponentListener (this);
	}
    
	columns.removeLast (numColumns - 1);
}

void ColumnFileBrowserContents::changeListenerCallback (ChangeBroadcaster* changedComponent)
{
    BrowserColumn* changedColumn = static_cast<BrowserColumn*> (changedComponent);
    
    columns[activeColumn]->setLookAndFeel (inactiveLookAndFeel);
    
    activeColumn = changedColumn->columnNumber;
    
    selectedFileChanged (changedColumn->getHighlightedFile());

    columns[activeColumn]->setLookAndFeel (activeLookAndFeel);
}

void ColumnFileBrowserContents::componentMovedOrResized (Component &component, bool wasMoved, bool wasResized)
{
    if (wasResized)
        resized();
}

bool ColumnFileBrowserContents::keyPressed (const KeyPress& key)
{
    if (key.isKeyCode (KeyPress::leftKey))
    {
        if (activeColumn != 0)
        {
            FileListComponent* list = dynamic_cast<FileListComponent*> (columns[activeColumn]->getDisplayComponent());
            list->deselectAllRows();
            
            int newActiveColumn = jmax (0, activeColumn - 1);
            columns[newActiveColumn]->selectionChanged();
            columns[newActiveColumn]->grabKeyboardFocus();
        }
        
        return true;
    }
    else if (key.isKeyCode (KeyPress::rightKey))
    {
        if (columns[activeColumn]->getNumSelectedFiles() == 1
            && columns[activeColumn]->getSelectedFile (0).isDirectory())
        {
            int newActiveColumn = activeColumn + 1;
            addColumn (columns[activeColumn]->getSelectedFile (0));
            
            FileListComponent* list = dynamic_cast<FileListComponent*> (columns[newActiveColumn]->getDisplayComponent());

            if (list->getNumRows() > 0)
            {
                columns[newActiveColumn]->grabKeyboardFocus();
                list->selectRow (0);
            }
        }
        
        return true;
    }
    
    return false;
}

//==================================================================================
ColumnFileBrowser::ColumnFileBrowser (WildcardFileFilter* filesToDisplay_)
    : Viewport("ColumnFileBrowser"),
      wildcard(filesToDisplay_)
{
    addMouseListener (this, true);
    setWantsKeyboardFocus (false);
    
    setScrollBarsShown (false, true);
    setScrollBarThickness (10);
    
    fileBrowser = new ColumnFileBrowserContents (filesToDisplay_, this);
    setViewedComponent (fileBrowser);
}

ColumnFileBrowser::~ColumnFileBrowser()
{
}

void ColumnFileBrowser::resized()
{
    Viewport::resized();
    fileBrowser->setSize (fileBrowser->getWidth(), getMaximumVisibleHeight());
}

void ColumnFileBrowser::visibleAreaChanged (const Rectangle<int>& newVisibleArea)
{
    resized();
}

void ColumnFileBrowser::mouseWheelMove (const MouseEvent& e, float wheelIncrementX, float wheelIncrementY)
{
    if (! (e.mods.isAltDown() || e.mods.isCtrlDown()))
    {
        if (getHorizontalScrollBar()->isVisible())
            Viewport::useMouseWheelMoveIfNeeded (e, wheelIncrementX, 0);
        else
            Viewport::useMouseWheelMoveIfNeeded (e, wheelIncrementX, wheelIncrementY);
    }
}

END_JUCE_NAMESPACE