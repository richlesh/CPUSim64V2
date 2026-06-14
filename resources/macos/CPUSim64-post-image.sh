#!/bin/bash
# Post-image script: add UTExportedTypeDeclarations to Info.plist so macOS
# properly registers custom file-type associations (especially .o64 which
# conflicts with the system Wavefront OBJ UTI).

PLIST="CPUSim64.app/Contents/Info.plist"
RESOURCES="CPUSim64.app/Contents/Resources"

# Copy document type icons into the app bundle's Resources directory
cp resources/asm_icon.icns "$RESOURCES/"
cp resources/o64_icon.icns "$RESOURCES/"
cp resources/sym_icon.icns "$RESOURCES/"

/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations array" "$PLIST"

# .asm
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:0 dict" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeIdentifier string com.richardlesh.cpusim64.asm" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeDescription string CPUSim64 Assembly File" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeIconFile string asm_icon.icns" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo:0 string public.plain-text" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension:0 string asm" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.mime-type string application/x-cpusim-asm" \
  "$PLIST"

# .o64 — no longer conflicts with Wavefront OBJ; use public.data conformance
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:1 dict" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeIdentifier string com.richardlesh.cpusim64.o64" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeDescription string CPUSim64 Object File" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeIconFile string o64_icon.icns" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.filename-extension:0 string o64" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.mime-type string application/x-cpusim-o64" \
  "$PLIST"

# .sym
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:2 dict" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeIdentifier string com.richardlesh.cpusim64.sym" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeDescription string CPUSim64 Symbol File" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeIconFile string sym_icon.icns" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeTagSpecification:public.filename-extension:0 string sym" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeTagSpecification:public.mime-type string application/x-cpusim-sym" \
  "$PLIST"

# .sym1
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:3 dict" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeIdentifier string com.richardlesh.cpusim64.sym1" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeDescription string CPUSim64 Symbol File" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeIconFile string sym_icon.icns" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeTagSpecification:public.filename-extension:0 string sym1" \
  -c "Add :UTExportedTypeDeclarations:3:UTTypeTagSpecification:public.mime-type string application/x-cpusim-sym1" \
  "$PLIST"

# .sym2
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:4 dict" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeIdentifier string com.richardlesh.cpusim64.sym2" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeDescription string CPUSim64 Symbol File" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeIconFile string sym_icon.icns" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.filename-extension:0 string sym2" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.mime-type string application/x-cpusim-sym2" \
  "$PLIST"

# Update CFBundleDocumentTypes to reference UTIs via LSItemContentTypes, set LSHandlerRank,
# and set CFBundleTypeIconFile (which Finder uses to display document icons)
TYPES_COUNT=$(/usr/libexec/PlistBuddy -c "Print :CFBundleDocumentTypes" "$PLIST" | grep -c "Dict")
for ((i=0; i<TYPES_COUNT; i++)); do
  EXT=$(/usr/libexec/PlistBuddy -c "Print :CFBundleDocumentTypes:$i:CFBundleTypeExtensions:0" "$PLIST" 2>/dev/null || echo "")
  case "$EXT" in
    asm)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.asm" "$PLIST"
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSHandlerRank string Owner" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:CFBundleTypeIconFile string asm_icon.icns" "$PLIST" 2>/dev/null
      ;;
    o64)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.o64" "$PLIST"
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSHandlerRank string Owner" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:CFBundleTypeIconFile string o64_icon.icns" "$PLIST" 2>/dev/null
      ;;
    sym)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym" "$PLIST"
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSHandlerRank string Owner" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:CFBundleTypeIconFile string sym_icon.icns" "$PLIST" 2>/dev/null
      ;;
    sym1)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym1" "$PLIST"
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSHandlerRank string Owner" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:CFBundleTypeIconFile string sym_icon.icns" "$PLIST" 2>/dev/null
      ;;
    sym2)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym2" "$PLIST"
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSHandlerRank string Owner" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:CFBundleTypeIconFile string sym_icon.icns" "$PLIST" 2>/dev/null
      ;;
  esac
done

echo "Info.plist patched with UTExportedTypeDeclarations, UTTypeIconFile, and LSHandlerRank"
