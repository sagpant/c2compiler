/* Copyright 2013-2019 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CGenerator/MakefileGenerator.h"
#include "CGenerator/DepSorter.h"
#include "AST/Component.h"
#include "Utils/StringBuilder.h"
#include "Utils/BuildFile.h"
#include "Utils/TargetInfo.h"
#include "FileUtils/FileUtils.h"
#include "Utils/StringList.h"

using namespace C2;

MakefileGenerator::MakefileGenerator(const Component& component_,
                                     bool singleFile_,
                                     const TargetInfo& targetInfo_,
                                     const BuildFile* buildFile_)
    : component(component_)
    , target(component.getName())
    , targetInfo(targetInfo_)
    , buildFile(buildFile_)
    , singleFile(singleFile_)
{}

void MakefileGenerator::write(const std::string& path) {
    StringList files;
    if (singleFile) {
        files.push_back(component.getName());
    } else {
        const ModuleList& mods = component.getModules();
        for (unsigned m=0; m<mods.size(); m++) {
            files.push_back(mods[m]->getName());
        }
    }

    std::string targetname = "../";
    std::string libname;
    switch (component.getType()) {
    case Component::EXECUTABLE:
        targetname += target;
        break;
    case Component::SHARED_LIB:
#ifdef __APPLE__
        targetname += "lib" + target + ".dylib";
        libname = "lib" + target + ".dylib";
#else
        targetname += "lib" + target + ".so";
        libname = "lib" + target + ".so";
#endif
        break;
    case Component::STATIC_LIB:
        targetname += "lib" + target + ".a";
        break;
    case Component::SOURCE_LIB:
        // nothing to do
        break;
    }

    StringBuilder out;
    out << "# WARNING: this file is auto-generated by the C2 compiler.\n";
    out << "# Any changes you make might be lost!\n\n";


    // CC
    if (buildFile && !buildFile->cc.empty()) {
        out << "CC=" << buildFile->cc;
    } else {
        out << "CC=gcc";
    }
    out << '\n';

    // CFLAGS
    StringBuilder cflags(256);
    if (buildFile && !buildFile->cflags.empty()) {
        cflags << buildFile->cflags;
    } else {
        cflags << "-Wall -Wextra -Wno-unused -Wno-switch";
        //cflags << " -Werror";
    }
    if (component.isSharedLib()) cflags << " -fPIC";
    cflags << " -pipe -O2 -std=c99";
    cflags << " -Wno-missing-field-initializers";
    out << "CFLAGS=" << cflags.c_str() << '\n';

    // LDFLAGS
    if (buildFile && !buildFile->ldflags.empty()) {
        out << "LDFLAGS=" << buildFile->ldflags << '\n';
    } else {
        out << "LDFLAGS=\n";
    }

    out << '\n';

    // all target
    out << "all: " << targetname << '\n';
    out << '\n';

    // our target
    out << targetname << ':';
    for (StringListConstIter iter=files.begin(); iter!=files.end(); ++iter) {
        out << ' ' << *iter << ".c";
    }
    out << '\n';

    // compile step
    for (StringListConstIter iter=files.begin(); iter!=files.end(); ++iter) {
        out << "\t$(CC) $(CFLAGS) -c " << *iter << ".c -o " << *iter << ".o\n";
    }

    // link step
    switch (component.getType()) {
    case Component::EXECUTABLE:
    {
        out << "\t$(CC) $(LDFLAGS) -o " << targetname;
        for (StringListConstIter iter=files.begin(); iter!=files.end(); ++iter) {
            out << ' ' << *iter << ".o";
        }

        DepSorter sorter(component);
        for (unsigned i=0; i<sorter.numComps(); ++i) {
            addLinkFlags(sorter.getComp(i), out);
        }
        break;
    }
    case Component::SHARED_LIB:
        out << "#link against with: $(CC) main.c -L. -l<libname> -o test\n";
        out << "\t$(CC) $(LDFLAGS)";
        for (StringListConstIter iter=files.begin(); iter!=files.end(); ++iter) {
            out << ' ' << *iter << ".o";
        }
#ifdef __APPLE__
        out << " -dynamiclib -Wl,-headerpad_max_install_names -o " << targetname;
        out << " -Wl,-install_name," << libname;
        // TODO export script replacement?
#else
        out << " -shared -o " << targetname << " -Wl,-soname," << libname << ".1";
        out << " -Wl,--version-script=exports.version";
#endif
        break;
    case Component::STATIC_LIB:
        out << "\tar rcs " << targetname;
        for (StringListConstIter iter=files.begin(); iter!=files.end(); ++iter) {
            out << ' ' << *iter << ".o";
        }
        break;
    case Component::SOURCE_LIB:
        break;
    }
    out << '\n';
    out << '\n';

    // show target (for export debug)
    out << "symbols:\n";
#ifdef __APPLE__
    out << "\t nm -U " << targetname << '\n';
#else
    out << "\t nm -g -D -C --defined-only " << targetname << '\n';
#endif
    out << '\n';

    // clean target
    out << "clean:\n";
    out << "\t rm -f *.o *.a " << targetname << '\n';
    out << '\n';

    FileUtils::writeFile(path.c_str(), path + "/Makefile", out);
}

void MakefileGenerator::addLinkFlags(const Component* dep, StringBuilder& out)
{
    // TEMP add -L flag if static lib (system static libs not yet supported)
    if (dep->isStaticLib()) {
        out << " -L" << dep->getPath() << '/' << Str(targetInfo);
    }
    if (dep->getLinkName() != "") {
        out << " -l" << dep->getLinkName();
    }
}

