/*
 * Copyright (C) 2018 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @tree.h
 * The tree classes, responsible for synchronizing multiple parts of Splash
 * It has the particularity of generating a Seed for every change applied to it,
 * which can then be used for replicating the change into another Tree
 */

#ifndef SPLASH_TREE_H
#define SPLASH_TREE_H

#include "./core/tree/tree_branch.h"
#include "./core/tree/tree_leaf.h"
#include "./core/tree/tree_root.h"

#endif // SPLASH_TREE_H
