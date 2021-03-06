#pragma once

#include <iostream>
#include <vector>
#include "Constants.h"


class PtTreeNode {
public:
	float x, y;
	int index;
	PtTreeNode* left = nullptr;
	PtTreeNode* right = nullptr;

	PtTreeNode(float& x, float& y, int& index);
};

class PtTree {
private:
	std::vector<PtTreeNode> nodes;

	PtTreeNode* root = nullptr;

public:
	PtTree();
	void setPoints(std::vector<VE::Point>& points);
	int nearest(const VE::Point& target, float& maxDist2);

	void kNearest(const VE::Point& target, float& maxDist2, int* indices, float * distances2, const int& amount);

};