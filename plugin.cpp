/*
**
* BEGIN_COPYRIGHT
*
* This file is part of SciDB.
* Copyright (C) 2008-2013 SciDB, Inc.
*
* SciDB is free software: you can redistribute it and/or modify
* it under the terms of the AFFERO GNU General Public License as published by
* the Free Software Foundation.
*
* SciDB is distributed "AS-IS" AND WITHOUT ANY WARRANTY OF ANY KIND,
* INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,
* NON-INFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE. See
* the AFFERO GNU General Public License for the complete license terms.
*
* You should have received a copy of the AFFERO GNU General Public License
* along with SciDB.  If not, see <http://www.gnu.org/licenses/agpl-3.0.html>
*
* END_COPYRIGHT
*/

/*
 * @file plugin.cpp
 * Stub file for a SciDB plugin.
 */

#include "vector"
#include "SciDBAPI.h"
#include "system/Constants.h"

/**
 * EXPORT FUNCTIONS
 * Functions from this section will be used by LOAD LIBRARY operator.
 */

/**
 * Returns the version of SciDB this plugin was written for.
 * SciDB will throw an error on load_library if this version is greater than the running SciDB version.
 */
EXPORTED_FUNCTION void GetPluginVersion(uint32_t& major, uint32_t& minor, uint32_t& patch, uint32_t& build)
{
    major = scidb::SCIDB_VERSION_MAJOR();
    minor = scidb::SCIDB_VERSION_MINOR();
    patch = scidb::SCIDB_VERSION_PATCH();
    build = scidb::SCIDB_VERSION_BUILD();
}
