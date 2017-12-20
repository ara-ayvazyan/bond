-- Copyright (c) Microsoft. All rights reserved.
-- Licensed under the MIT license. See LICENSE file in the project root for full license information.

{-# LANGUAGE QuasiQuotes, OverloadedStrings, RecordWildCards #-}

module Language.Bond.Codegen.Cpp.Reflection_h (reflection_h) where

import System.FilePath
import Data.Monoid
import Prelude
import Data.Text.Lazy (Text)
import qualified Data.Foldable as F
import Text.Shakespeare.Text
import Language.Bond.Util
import Language.Bond.Syntax.Types
import Language.Bond.Codegen.TypeMapping
import Language.Bond.Codegen.Util
import qualified Language.Bond.Codegen.Cpp.Util as CPP

-- | Codegen template for generating /base_name/_reflection.h containing schema
-- metadata definitions.
reflection_h :: Maybe String -> MappingContext -> String -> [Import] -> [Declaration] -> (String, Text)
reflection_h export_attribute cpp file imports declarations = ("_reflection.h", [lt|
#pragma once

#include "#{file}_types.h"
#include <bond/core/reflection.h>
#{newlineSepEnd 0 include imports}
#{CPP.openNamespace cpp}
    #{doubleLineSepEnd 1 schema declarations}
#{CPP.closeNamespace cpp}
|])
  where
    idl = MappingContext idlTypeMapping [] [] []  

    -- C++ type
    cppType = getTypeName cpp

    -- template for generating #include statement from import
    include (Import path) = [lt|#include "#{dropExtension path}_reflection.h"|]

    -- template for generating struct schema
    schema s@Struct {..} = [lt|//
    // #{declName}
    //
    #{CPP.template s}struct #{className}::Schema
    {
        typedef #{baseType structBase} base;

        #{export_attr}static const ::bond::Metadata metadata;
        #{newlineBeginSep 2 fieldMetadata structFields}

        public: struct var
        {#{fieldTemplates indexedFields}};

        using field_count = std::integral_constant<uint16_t, #{length structFields}>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        #{fieldIds indexedFields}
        #{constructor}
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit#{metadataInitArgs}("#{declName}", "#{getDeclTypeName idl s}",
                #{CPP.attributeInit declAttributes}
            );
        }
    };
    #{onlyTemplate $ CPP.schemaMetadata cpp s}|]
      where
        classParams = CPP.classParams s

        className = CPP.className s

        export_attr = onlyNonTemplate $ optional (\a -> [lt|#{a}
        |]) export_attribute

        onlyTemplate x = if null declParams then mempty else x
        onlyNonTemplate x = if null declParams then x else mempty

        metadataInitArgs = onlyTemplate [lt|<::bond::detail::mpl::list#{classParams} >|]

        typename = onlyTemplate [lt|typename |]

        -- constructor, generated only for struct templates
        constructor = onlyTemplate [lt|
        Schema()
        {
            // Force instantiation of template statics
            (void)metadata;
            #{newlineSep 3 static structFields}
        }
        |]
          where
            static Field {..} = [lt|(void)s_#{fieldName}_metadata;|]
        
        -- list of fields zipped with indexes
        indexedFields :: [(Field, Int)]
        indexedFields = zip structFields [0..]

        baseType (Just base) = cppType base
        baseType Nothing = "::bond::no_base"

        fieldIds = F.foldMap $ \ (Field {..}, i) -> [lt|template <int __bond_dummy> struct field<#{i}, __bond_dummy> : ::bond::detail::mpl::identity<#{typename}var::#{fieldName}> {};
        |]

        fieldMetadata Field {..} =
            [lt|private: #{export_attr}static const ::bond::Metadata s_#{fieldName}_metadata;|]

        fieldTemplates = F.foldMap $ \ (f@Field {..}, i) -> [lt|
            // #{fieldName}
            typedef struct : ::bond::reflection::FieldTemplate<
                #{fieldOrdinal},
                #{i},
                #{CPP.modifierTag f},
                #{className},
                #{cppType fieldType},
                &#{className}::#{fieldName},
                &s_#{fieldName}_metadata
            > {} #{fieldName};
        |]


    -- nothing to generate for enums
    schema _ = mempty
