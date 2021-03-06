/**
 *    Copyright (C) 2013 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <boost/scoped_ptr.hpp>
#include <vector>

#include "mongo/db/diskloc.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/matcher.h"
#include "mongo/db/exec/plan_stage.h"
#include "mongo/platform/unordered_set.h"

namespace mongo {

    /**
     * Reads from N children, each of which must have a valid DiskLoc.  Uses a hash table to
     * intersect the outputs of the N children, and outputs the intersection.
     *
     * Preconditions: Valid DiskLoc.  More than one child.
     *
     * Any DiskLoc that we keep a reference to that is invalidated before we are able to return it
     * is fetched and added to the WorkingSet as "flagged for further review."  Because this stage
     * operates with DiskLocs, we are unable to evaluate the AND for the invalidated DiskLoc, and it
     * must be fully matched later.
     */
    class AndHashStage : public PlanStage {
    public:
        AndHashStage(WorkingSet* ws, Matcher* matcher);
        virtual ~AndHashStage();

        void addChild(PlanStage* child);

        virtual StageState work(WorkingSetID* out);
        virtual bool isEOF();

        virtual void prepareToYield();
        virtual void recoverFromYield();
        virtual void invalidate(const DiskLoc& dl);

    private:
        StageState readFirstChild();
        StageState hashOtherChildren();

        // Not owned by us.
        WorkingSet* _ws;
        scoped_ptr<Matcher> _matcher;

        // The stages we read from.  Owned by us.
        vector<PlanStage*> _children;

        // _dataMap is filled out by the first child and probed by subsequent children.
        typedef unordered_map<DiskLoc, WorkingSetID, DiskLoc::Hasher> DataMap;
        DataMap _dataMap;

        // Keeps track of what elements from _dataMap subsequent children have seen.
        typedef unordered_set<DiskLoc, DiskLoc::Hasher> SeenMap;
        SeenMap _seenMap;

        // Iterator over the members of _dataMap that survive.
        DataMap::iterator _resultIterator;

        // True if we're still scanning _children for results.
        bool _shouldScanChildren;

        // Which child are we currently working on?
        size_t _currentChild;
    };

}  // namespace mongo
