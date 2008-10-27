/*
    Copyright (c) 2008 NetAllied Systems GmbH

    This file is part of COLLADAMaya.

    Portions of the code are:
    Copyright (c) 2005-2007 Feeling Software Inc.
    Copyright (c) 2005-2007 Sony Computer Entertainment America
    Copyright (c) 2004-2005 Alias Systems Corp.

    Licensed under the MIT Open Source License, 
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/

#include "COLLADAMayaStableHeaders.h"
#include "COLLADAMayaDocumentImporter.h"
#include "COLLADAMayaReferenceManager.h"
#include "COLLADAMayaImportOptions.h"
#include "COLLADAMayaMaterialImporter.h"
#include "COLLADAMayaGeometryImporter.h"
#include "COLLADAMayaCameraImporter.h"
#include "COLLADAMayaVisualSceneImporter.h"

#include "COLLADASWURI.h"

#include "COLLADADocumentUtil.h"

#include "dom/domTypes.h"


namespace COLLADAMaya
{
    
    //---------------------------------------------------------------
    DocumentImporter::DocumentImporter ( const String& fileName )
        : mFileName ( fileName )
        , mMaterialImporter ( NULL )
        , mGeometryImporter ( NULL )
        , mCameraImporter ( NULL )
        , mVisualSceneImporter ( NULL )
        , mSceneId ( "MayaScene" )
    {
    }

    //---------------------------------------------------------------
    DocumentImporter::~DocumentImporter()
    {
        releaseLibraries(); 
    }

    //---------------------------------------------------------------
    void DocumentImporter::createLibraries()
    {
        releaseLibraries();

        // Get the sceneID (assign a name to the scene)
        MString sceneName;
        MGlobal::executeCommand ( MString ( "file -q -ns" ), sceneName );
        if ( sceneName.length() != 0 ) mSceneId = sceneName.asChar();

        // Initialize the reference manager
        ReferenceManager::getInstance()->initialize ();

        // Create the basic elements
        mMaterialImporter = new MaterialImporter ( this );
        mGeometryImporter = new GeometryImporter ();
        mCameraImporter = new CameraImporter ();
        mVisualSceneImporter = new VisualSceneImporter ();
    }

    //---------------------------------------------------------------
    void DocumentImporter::releaseLibraries()
    {
        delete mMaterialImporter;
        delete mGeometryImporter;
        delete mCameraImporter;
        delete mVisualSceneImporter;
    }

    //-----------------------------
    void DocumentImporter::importCurrentScene()
    {
        // Create the import/export library helpers.
        createLibraries();

        DAE dae;
        String fileUriString = COLLADASW::URI::nativePathToUri( getFilename() );
        mColladaDocument = dae.open ( fileUriString );

        mDaeDocument = mColladaDocument->getDocument();

        // Import the asset information.
        importAsset ();

        // Import the DAG entity libraries
        mMaterialImporter->importMaterials();

//        mCameraImporter->Import();
//        mLightImporter->Import();
//        mGeometryImporter->Import();
//        controllerLibrary->Import();
//        mVisualSceneImporter->Import();

    }

    //---------------------------------
    void DocumentImporter::importAsset()
    {
        // Up_axis
        if ( MGlobal::mayaState() != MGlobal::kBatch )
        {
            if ( ImportOptions::isOpenMode() && ImportOptions::importUpAxis() )
            {
                char upAxis = 'y';
                
                daeDocument* daeDoc = mColladaDocument->getDocument();
                domUpAxisType upAxisType = COLLADA::DocumentUtil::getUpAxis ( daeDoc );
                switch ( upAxisType )
                {
                case UPAXISTYPE_Z_UP: upAxis = 'z';
                case UPAXISTYPE_Y_UP: upAxis = 'y';
                case UPAXISTYPE_X_UP:
                    MGlobal::displayWarning ( "An up_axis of 'X' is not supported by Maya." );
                default:
                    MGlobal::displayWarning ( "Unknown up_axis value." );
                }

                // Use the MEL commands to set the up_axis. Currently resets the view, if the axis must change..
                MString command ( "string $currentAxis = `upAxis -q -ax`; if ($currentAxis != \"" );
                command += upAxis; command += "\") { upAxis -ax \""; command += upAxis;
                command += "\"; viewSet -home persp; }";
                MGlobal::executeCommand ( command );
            }
        }

        // Retrieve Maya's current up-axis.
        MString result;
//        FMVector3 mayaUpAxis = FMVector3::Zero;
        if ( ImportOptions::importUpAxis() )
        {
            MGlobal::executeCommand ( "upAxis -q -ax;", result, false, false );
            // TODO
//             if ( result == "z" )
// 
//             mayaUpAxis = FMVector3::YAxis;
//             if (IsEquivalent(MConvert::ToFChar(result), FC("z"))) mayaUpAxis = FMVector3::ZAxis;
        }

        float mayaUnit = 0.0f;
        if ( ImportOptions::importUnits() ) mayaUnit = 0.01f;

        // TODO 
        // Standardize the COLLADA document on this up-axis and units (centimeters).
//        FCDocumentTools::StandardizeUpAxisAndLength(colladaDocument, mayaUpAxis, mayaUnit);

        // Get the UI unit factor, for parts of Maya that don't handle variable lengths correctly
        MDistance testDistance ( 1.0f, MDistance::uiUnit() );
        float uiUnitFactor = (float) testDistance.as ( MDistance::kCentimeters );
    }

    //---------------------------------
    const String& DocumentImporter::getFilename() const
    {
        return mFileName;
    }

    //---------------------------------
    GeometryImporter* DocumentImporter::getGeometryImporter()
    {
        return mGeometryImporter;
    }

    //---------------------------------
    VisualSceneImporter* DocumentImporter::getVisualSceneImporter()
    {
        return mVisualSceneImporter;
    }


}