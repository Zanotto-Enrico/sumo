/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.dev/sumo
// Copyright (C) 2001-2023 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNEWalk.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Jun 2019
///
// A class for visualizing walks in Netedit
/****************************************************************************/

#include <utils/gui/windows/GUIAppEnum.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNENet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNEViewNet.h>
#include <utils/gui/div/GUIDesigns.h>
#include <utils/gui/div/GLHelper.h>

#include "GNEWalk.h"
#include "GNERoute.h"

// ===========================================================================
// method definitions
// ===========================================================================

GNEWalk::GNEWalk(SumoXMLTag tag, GNENet* net) :
    GNEDemandElement("", net, GLO_WALK, tag, GUIIconSubSys::getIcon(GUIIcon::WALK_FROMTO),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {}, {}, {}, {}, {}),
    GNEDemandElementPlan(this, -1) {
    // reset default values
    resetDefaultValues();
}


GNEWalk::GNEWalk(GNENet* net, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, GNE_TAG_WALK_EDGE, GUIIconSubSys::getIcon(GUIIcon::WALK_FROMTO),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {fromEdge, toEdge}, {}, {}, {personParent}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::GNEWalk(const bool isTrain, GNENet* net, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEAdditional* toAdditional, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, isTrain ? GNE_TAG_WALK_TRAINSTOP : GNE_TAG_WALK_BUSSTOP, GUIIconSubSys::getIcon(isTrain ? GUIIcon::WALK_TRAINSTOP : GUIIcon::WALK_BUSSTOP),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {fromEdge}, {}, {toAdditional}, {personParent}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::GNEWalk(GNENet* net, GNEDemandElement* personParent, std::vector<GNEEdge*> edges, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, GNE_TAG_WALK_EDGES, GUIIconSubSys::getIcon(GUIIcon::WALK_EDGES),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {edges}, {}, {}, {personParent}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::GNEWalk(GNENet* net, GNEDemandElement* personParent, GNEDemandElement* route, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, GNE_TAG_WALK_ROUTE, GUIIconSubSys::getIcon(GUIIcon::WALK_ROUTE),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {}, {}, {}, {personParent, route}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::GNEWalk(GNENet* net, GNEDemandElement* personParent, GNEJunction* fromJunction, GNEJunction* toJunction, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, GNE_TAG_WALK_JUNCTIONS, GUIIconSubSys::getIcon(GUIIcon::WALK_JUNCTIONS),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {fromJunction, toJunction}, {}, {}, {}, {personParent}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::GNEWalk(GNENet* net, GNEDemandElement* personParent, GNEAdditional* fromTAZ, GNEAdditional* toTAZ, double arrivalPosition) :
    GNEDemandElement(personParent, net, GLO_WALK, GNE_TAG_WALK_TAZS, GUIIconSubSys::getIcon(GUIIcon::WALK_TAZS),
                     GNEPathManager::PathElement::Options::DEMAND_ELEMENT, {}, {}, {}, {fromTAZ, toTAZ}, {personParent}, {}),
    GNEDemandElementPlan(this, arrivalPosition) {
}


GNEWalk::~GNEWalk() {}


GNEMoveOperation*
GNEWalk::getMoveOperation() {
    // avoid move person plan that ends in busStop or junction
    if ((getParentAdditionals().size() > 0) || (getParentJunctions().size() > 0)) {
        return nullptr;
    }
    // get geometry end pos
    const Position geometryEndPos = getPathElementArrivalPos();
    // calculate circle width squared
    const double circleWidthSquared = myPersonPlanArrivalPositionDiameter * myPersonPlanArrivalPositionDiameter;
    // check if we clicked over a geometry end pos
    if (myNet->getViewNet()->getPositionInformation().distanceSquaredTo2D(geometryEndPos) <= ((circleWidthSquared + 2))) {
        // continue depending of parent edges
        if (getParentEdges().size() > 0) {
            return new GNEMoveOperation(this, getParentEdges().back()->getLaneByAllowedVClass(getVClass()), myArrivalPosition, false);
        } else {
            return new GNEMoveOperation(this, getParentDemandElements().at(1)->getParentEdges().back()->getLaneByAllowedVClass(getVClass()), myArrivalPosition, false);
        }
    } else {
        return nullptr;
    }
}


GUIGLObjectPopupMenu*
GNEWalk::getPopUpMenu(GUIMainWindow& app, GUISUMOAbstractView& parent) {
    GUIGLObjectPopupMenu* ret = new GUIGLObjectPopupMenu(app, parent, *this);
    // build header
    buildPopupHeader(ret, app);
    // build menu command for center button and copy cursor position to clipboard
    buildCenterPopupEntry(ret);
    buildPositionCopyEntry(ret, app);
    // build menu commands for names
    GUIDesigns::buildFXMenuCommand(ret, "Copy " + getTagStr() + " name to clipboard", nullptr, ret, MID_COPY_NAME);
    GUIDesigns::buildFXMenuCommand(ret, "Copy " + getTagStr() + " typed name to clipboard", nullptr, ret, MID_COPY_TYPED_NAME);
    new FXMenuSeparator(ret);
    // build selection and show parameters menu
    myNet->getViewNet()->buildSelectionACPopupEntry(ret, this);
    buildShowParamsPopupEntry(ret);
    // show option to open demand element dialog
    if (myTagProperty.hasDialog()) {
        GUIDesigns::buildFXMenuCommand(ret, ("Open " + getTagStr() + " Dialog").c_str(), getACIcon(), &parent, MID_OPEN_ADDITIONAL_DIALOG);
        new FXMenuSeparator(ret);
    }
    GUIDesigns::buildFXMenuCommand(ret, ("Cursor position in view: " + toString(getPositionInView().x()) + "," + toString(getPositionInView().y())).c_str(), nullptr, nullptr, 0);
    return ret;
}


void
GNEWalk::writeDemandElement(OutputDevice& device) const {
    // open tag
    device.openTag(SUMO_TAG_WALK);
    // write attributes depending  of walk type
    if (myTagProperty.getTag() == GNE_TAG_WALK_ROUTE) {
        device.writeAttr(SUMO_ATTR_ROUTE, getParentDemandElements().at(1)->getID());
    } else if (myTagProperty.getTag() == GNE_TAG_WALK_EDGES) {
        device.writeAttr(SUMO_ATTR_EDGES, parseIDs(getParentEdges()));
    } else {
        // check if from attribute is enabled
        if (isAttributeEnabled(SUMO_ATTR_FROM)) {
            // check if write edge or junction
            if (getParentEdges().size() > 0) {
                device.writeAttr(SUMO_ATTR_FROM, getParentEdges().front()->getID());
            } else if (getParentJunctions().size() > 0) {
                device.writeAttr(SUMO_ATTR_FROM_JUNCTION, getParentJunctions().front()->getID());
            } else if (getParentJunctions().size() > 0) {
                device.writeAttr(SUMO_ATTR_FROM_TAZ, getParentAdditionals().front()->getID());
            }
        }
        // write to depending if personplan ends in a busStop, edge or junction
        if (getParentAdditionals().size() > 0) {
            auto toAdditional = getParentAdditionals().back();
            if (toAdditional->getTagProperty().getTag() == SUMO_TAG_BUS_STOP) {
                device.writeAttr(SUMO_ATTR_BUS_STOP, toAdditional->getID());
            } else if (toAdditional->getTagProperty().getTag() == SUMO_TAG_TRAIN_STOP) {
                device.writeAttr(SUMO_ATTR_TRAIN_STOP, toAdditional->getID());
            } else {
                device.writeAttr(SUMO_ATTR_TO_TAZ, toAdditional->getID());
            }
        } else if (getParentEdges().size() > 0) {
            device.writeAttr(SUMO_ATTR_TO, getParentEdges().back()->getID());
        } else if (getParentJunctions().size() > 0) {
            device.writeAttr(SUMO_ATTR_TO_JUNCTION, getParentJunctions().back()->getID());
        }
    }
    // avoid write arrival positions in walk to busStop
    if ((myTagProperty.getTag() != GNE_TAG_RIDE_BUSSTOP) && (myTagProperty.getTag() != GNE_TAG_RIDE_TRAINSTOP) &&
            (myArrivalPosition > 0)) {
        device.writeAttr(SUMO_ATTR_ARRIVALPOS, myArrivalPosition);
    }
    // close tag
    device.closeTag();
}


GNEDemandElement::Problem
GNEWalk::isDemandElementValid() const {
    return isPersonPlanValid();
}


std::string
GNEWalk::getDemandElementProblem() const {
    return getPersonPlanProblem();
}


void
GNEWalk::fixDemandElementProblem() {
    // currently the only solution is removing Walk
}


SUMOVehicleClass
GNEWalk::getVClass() const {
    return getParentDemandElements().front()->getVClass();
}


const RGBColor&
GNEWalk::getColor() const {
    return getParentDemandElements().front()->getColor();
}


void
GNEWalk::updateGeometry() {
    updatePlanGeometry();
}


Position
GNEWalk::getPositionInView() const {
    return getPlanPositionInView();
}


std::string
GNEWalk::getParentName() const {
    return getParentDemandElements().front()->getID();
}


Boundary
GNEWalk::getCenteringBoundary() const {
    Boundary walkBoundary;
    // return the combination of all parent edges's boundaries
    for (const auto& i : getParentEdges()) {
        walkBoundary.add(i->getCenteringBoundary());
    }
    // check if is valid
    if (walkBoundary.isInitialised()) {
        return walkBoundary;
    } else {
        return Boundary(-0.1, -0.1, 0.1, 0.1);
    }
}


void
GNEWalk::splitEdgeGeometry(const double /*splitPosition*/, const GNENetworkElement* originalElement, const GNENetworkElement* newElement, GNEUndoList* undoList) {
    // only split geometry of WalkEdges
    if (myTagProperty.getTag() == GNE_TAG_WALK_EDGES) {
        // obtain new list of walk edges
        std::string newWalkEdges = getNewListOfParents(originalElement, newElement);
        // update walk edges
        if (newWalkEdges.size() > 0) {
            setAttribute(SUMO_ATTR_EDGES, newWalkEdges, undoList);
        }
    }
}


void
GNEWalk::drawGL(const GUIVisualizationSettings& s) const {
    drawPlanGL(s, s.colorSettings.walkColor);
}


void
GNEWalk::computePathElement() {
    // avoid calculate for junctions
    // calculate path depending of parents
    if (getParentJunctions().size() > 0) {
        // get previous personTrip
        const auto previousParent = getParentDemandElements().at(0)->getPreviousChildDemandElement(this);
        // calculate path
        if (previousParent == nullptr) {
            myNet->getPathManager()->calculatePathJunctions(this, getVClass(), getParentJunctions());
        } else if (previousParent->getParentJunctions().size() > 0) {
            myNet->getPathManager()->calculatePathJunctions(this, getVClass(), {previousParent->getParentJunctions().front(), getParentJunctions().back()});
        } else {
            myNet->getPathManager()->calculatePathJunctions(this, getVClass(), {previousParent->getLastPathLane()->getParentEdge()->getToJunction(), getParentJunctions().back()});
        }
    } else {
        // declare lane vector
        std::vector<GNELane*> lanes;
        // update lanes depending of walk tag
        if (myTagProperty.getTag() == GNE_TAG_WALK_EDGES) {
            // calculate consecutive path using parent edges
            myNet->getPathManager()->calculateConsecutivePathEdges(this, getVClass(), getParentEdges());
        } else if (myTagProperty.getTag() == GNE_TAG_WALK_ROUTE) {
            // calculate consecutive path using route edges
            myNet->getPathManager()->calculateConsecutivePathEdges(this, getVClass(), getParentDemandElements().back()->getParentEdges());
        } else if (getParentEdges().size() > 0) {
            // get first and last person plane
            lanes = {getFirstPathLane(), getLastPathLane()};
            // calculate path
            myNet->getPathManager()->calculatePathLanes(this, getVClass(), lanes);
        }
    }
    // update geometry
    updateGeometry();
}


void
GNEWalk::drawPartialGL(const GUIVisualizationSettings& s, const GNELane* lane, const GNEPathManager::Segment* segment, const double offsetFront) const {
    // draw person plan over lane
    drawPlanPartial(drawPersonPlan(), s, lane, segment, offsetFront, s.widthSettings.walkWidth, s.colorSettings.walkColor);
}


void
GNEWalk::drawPartialGL(const GUIVisualizationSettings& s, const GNELane* fromLane, const GNELane* toLane, const GNEPathManager::Segment* segment, const double offsetFront) const {
    // draw person plan over junction
    drawPlanPartial(drawPersonPlan(), s, fromLane, toLane, segment, offsetFront, s.widthSettings.walkWidth, s.colorSettings.walkColor);
}


GNELane*
GNEWalk::getFirstPathLane() const {
    // check if this walk is over a route
    if (myTagProperty.getTag() == GNE_TAG_WALK_ROUTE) {
        return getParentDemandElements().at(1)->getParentEdges().front()->getLaneByAllowedVClass(SVC_PEDESTRIAN);
    } else if (getParentJunctions().size() > 0) {
        throw ProcessError(TL("This walk use junctions"));
    } else {
        return getParentEdges().front()->getLaneByAllowedVClass(SVC_PEDESTRIAN);
    }
}


GNELane*
GNEWalk::getLastPathLane() const {
    // check if this walk is over a route
    if (myTagProperty.getTag() == GNE_TAG_WALK_ROUTE) {
        return getParentDemandElements().at(1)->getParentEdges().back()->getLaneByAllowedVClass(SVC_PEDESTRIAN);
    } else if (getParentAdditionals().size() > 0) {
        return getParentAdditionals().front()->getParentLanes().front();
    } else if (getParentJunctions().size() > 0) {
        throw ProcessError(TL("This walk use junctions"));
    } else {
        return getParentEdges().back()->getLaneByDisallowedVClass(SVC_PEDESTRIAN);
    }
}


std::string
GNEWalk::getAttribute(SumoXMLAttr key) const {
    return getPlanAttribute(key);
}


double
GNEWalk::getAttributeDouble(SumoXMLAttr key) const {
    return getPlanAttributeDouble(key);
}


Position
GNEWalk::getAttributePosition(SumoXMLAttr key) const {
    return getPlanAttributePosition(key);
}


void
GNEWalk::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    setPlanAttribute(key, value, undoList);
}


bool
GNEWalk::isValid(SumoXMLAttr key, const std::string& value) {
    return isPlanValid(key, value);
}


bool
GNEWalk::isAttributeEnabled(SumoXMLAttr key) const {
    return isPlanAttributeEnabled(key);
}


std::string
GNEWalk::getPopUpID() const {
    return getTagStr();
}


std::string
GNEWalk::getHierarchyName() const {
    return getPlanHierarchyName();
}


const Parameterised::Map&
GNEWalk::getACParametersMap() const {
    return getParametersMap();
}

// ===========================================================================
// private
// ===========================================================================

void
GNEWalk::setAttribute(SumoXMLAttr key, const std::string& value) {
    setPlanAttribute(key, value);
}


void
GNEWalk::setMoveShape(const GNEMoveResult& moveResult) {
    // change both position
    myArrivalPosition = moveResult.newFirstPos;
    // update geometry
    updateGeometry();
}


void
GNEWalk::commitMoveShape(const GNEMoveResult& moveResult, GNEUndoList* undoList) {
    undoList->begin(this, "arrivalPos of " + getTagStr());
    // now adjust start position
    setAttribute(SUMO_ATTR_ARRIVALPOS, toString(moveResult.newFirstPos), undoList);
    undoList->end();
}

/****************************************************************************/
