QHexEdit
========
QHexEdit is a hexadecimal widget for Qt5

<p align="center">
<img height="400" src="https://raw.githubusercontent.com/Dax89/QHexEdit/master/screenshots/QHexEdit.png">
</p>

Features
-----
- Partial large file support (Gap Buffer algorithm).
- Unlimited Undo/Redo.
- Fully Customizable.
- Easy to Use.

Usage
-----
The data is managed by QHexEdit through QHexDocument class.<br>
You can load a generic QIODevice using the method fromDevice() of QHexDocument class, also, helper methods are provided in order to load a QFile a In-Memory buffer in QHexEdit:<br>
```
// Load data from In-Memory Buffer...
QHexDocument* document = QHexDocument::fromMemory(bytearray);

// ...from a generic I/O device...
QHexDocument* document = QHexDocument::fromDevice(iodevice);

/* ...or from File */
QHexDocument* document = QHexDocument::fromFile("data.bin");

QHexEdit* hexedit = new QHexEdit();
hexedit->setDocument(document);                            // Associate QHexEditData with this QHexEdit

// Metatadata management
document->highlightBackRange(0, 10, QColor(Qt::Red));      // Highlight background from 0 to 10 (10 included) 
document->highlightForeRange(0, 15, QColor(Qt::darkBLue)); // Highlight foreground from 0 to 15 (15 included)
document->clearHighlighting();                             // Clear Highlighting 
document->commentRange(0, 12, "I'm a comment!");           // Add a comment 
document->clearComments();                                 // Clear all comments

// Bulk metadata management (paints only one time)
document->beginMetadata();                                 // Lock repaint
document->highlightBackRange(0, 10, QColor(Qt::Red));      // Highlight background from 0 to 10 (10 included) 
document->highlightForeRange(0, 15, QColor(Qt::darkBLue)); // Highlight foreground from 0 to 15 (15 included) 
document->clearHighlighting();                             // Clear Highlighting 
document->commentRange(0, 12, "I'm a comment!");           // Add a comment 
document->clearComments();                                 // Clear all comments
document->endMetadata();                                   // Unlock repaint

// Data editing
QByteArray data = document->read(24, 78);                // Read 78 bytes starting from offset 24
document->insert(4, QString("Hello QHexEdit").toUTF8()); // Insert an UTF-8 string from offset 4 
document->remove(6, 10);                                 // Delete bytes from offset 6 to offset 10 
document->replace(30, 12, QString("New Data").toUTF8()); // Replace 12 bytes from offset 30 with the UTF-8 string "New Data"
```

License
-----
QHexEdit is released under MIT license
