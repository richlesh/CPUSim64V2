#!/bin/bash
# Post-image script: add UTExportedTypeDeclarations to Info.plist so macOS
# properly registers custom file-type associations (especially .obj which
# conflicts with the system Wavefront OBJ UTI).

PLIST="CPUSim64.app/Contents/Info.plist"

/usr/libexec/PlistBuddy -c "Add :UTExportedTypeDeclarations array" "$PLIST"

# .asm
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:0 dict" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeIdentifier string com.richardlesh.cpusim64.asm" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeDescription string CPUSim64 Assembly File" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeConformsTo:0 string public.plain-text" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.filename-extension:0 string asm" \
  -c "Add :UTExportedTypeDeclarations:0:UTTypeTagSpecification:public.mime-type string application/x-cpusim-asm" \
  "$PLIST"

# .obj
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:1 dict" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeIdentifier string com.richardlesh.cpusim64.obj" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeDescription string CPUSim64 Object File" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.filename-extension:0 string obj" \
  -c "Add :UTExportedTypeDeclarations:1:UTTypeTagSpecification:public.mime-type string application/x-cpusim-obj" \
  "$PLIST"

# .sym
/usr/libexec/PlistBuddy \
  -c "Add :UTExportedTypeDeclarations:2 dict" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeIdentifier string com.richardlesh.cpusim64.sym" \
  -c "Add :UTExportedTypeDeclarations:2:UTTypeDescription string CPUSim64 Symbol File" \
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
  -c "Add :UTExportedTypeDeclarations:4:UTTypeConformsTo array" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeConformsTo:0 string public.data" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification dict" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.filename-extension array" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.filename-extension:0 string sym2" \
  -c "Add :UTExportedTypeDeclarations:4:UTTypeTagSpecification:public.mime-type string application/x-cpusim-sym2" \
  "$PLIST"

# Also update CFBundleDocumentTypes to reference UTIs via LSItemContentTypes
# This ensures macOS uses the UTI-based matching rather than just extension-based
TYPES_COUNT=$(/usr/libexec/PlistBuddy -c "Print :CFBundleDocumentTypes" "$PLIST" | grep -c "Dict")
for ((i=0; i<TYPES_COUNT; i++)); do
  EXT=$(/usr/libexec/PlistBuddy -c "Print :CFBundleDocumentTypes:$i:CFBundleTypeExtensions:0" "$PLIST" 2>/dev/null || echo "")
  case "$EXT" in
    asm)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.asm" "$PLIST"
      ;;
    obj)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.obj" "$PLIST"
      ;;
    sym)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym" "$PLIST"
      ;;
    sym1)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym1" "$PLIST"
      ;;
    sym2)
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes array" "$PLIST" 2>/dev/null
      /usr/libexec/PlistBuddy -c "Add :CFBundleDocumentTypes:$i:LSItemContentTypes:0 string com.richardlesh.cpusim64.sym2" "$PLIST"
      ;;
  esac
done

echo "Info.plist patched with UTExportedTypeDeclarations and LSItemContentTypes"
