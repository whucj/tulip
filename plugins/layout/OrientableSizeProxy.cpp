#include "OrientableSizeProxy.h"

//====================================================================
OrientableSizeProxy::OrientableSizeProxy(Sizes* sizesProxyParam,
                                         orientationType mask) : 
    sizesProxy(sizesProxyParam) {
    setOrientation(mask);
}

//====================================================================
void OrientableSizeProxy::setOrientation(orientationType mask)
{
	orientation = mask;
	  
    readW  = &Size::getW;
    readH  = &Size::getH;
    readD  = &Size::getD;
    writeW = &Size::setW;
    writeH = &Size::setH;
    writeD = &Size::setD;

    if (orientation & ORI_ROTATION_XY) {
        std::swap(readW, readH);
        std::swap(writeW, writeH);
    }    
}

//====================================================================
OrientableSize OrientableSizeProxy::createSize(const float width,
                                               const float height,
                                               const float depth) {
    return OrientableSize(this, width, height, depth);
}

//====================================================================
OrientableSize OrientableSizeProxy::createSize(const Size& v) {
    return OrientableSize(this, v);
}

//====================================================================
void OrientableSizeProxy::setAllNodeValue(const PointType& v) {
    sizesProxy->setAllNodeValue(v);
}

//====================================================================
void OrientableSizeProxy::setNodeValue(node n, const PointType& v) {
    sizesProxy->setNodeValue(n, v);
}

//====================================================================
OrientableSizeProxy::PointType OrientableSizeProxy::getNodeValue(const node n) {
    return OrientableSize(this, sizesProxy->getNodeValue(n));
}

//====================================================================
OrientableSizeProxy::PointType OrientableSizeProxy::getNodeDefaultValue() {
    return OrientableSize(this, sizesProxy->getNodeDefaultValue());
}

//====================================================================
void OrientableSizeProxy::setAllEdgeValue(const LineType& v) {
    sizesProxy->setAllEdgeValue(v);
}

//====================================================================
void OrientableSizeProxy::setEdgeValue(const edge e, const LineType& v) {
    sizesProxy->setEdgeValue(e, v);
}

//====================================================================
OrientableSizeProxy::LineType OrientableSizeProxy::getEdgeValue(const edge e) {
    return OrientableSize(this, sizesProxy->getEdgeValue(e));
}

//====================================================================
OrientableSizeProxy::LineType OrientableSizeProxy::getEdgeDefaultValue() {
    return OrientableSize(this, sizesProxy->getEdgeDefaultValue());
}
