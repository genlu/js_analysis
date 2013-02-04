/*
 * df.h
 *
 * computation of dominance frontier, both forward and on reverse CFG
 *
 *  Created on: Jan 31, 2013
 *      Author: genlu
 */

#ifndef DF_H_
#define DF_H_

ArrayList *addAugmentedExitBlocks(ArrayList *blockList);
void 	removeAugmentedExitBlocks(ArrayList *blockList, ArrayList *augmentedBlocks);
void findReverseDominators(ArrayList *blockList);
void computeReverseDominanceFrontier(ArrayList *blockList);

#endif /* DF_H_ */
