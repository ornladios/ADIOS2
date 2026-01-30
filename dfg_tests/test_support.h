#ifndef DFG_TEST_SUPPORT_H
#define DFG_TEST_SUPPORT_H

#include "evpath.h"
#include "support.h"
#include "simple_rec.h"

/* DFG tests implement these functions */
extern int be_test_master(int argc, char **argv);
extern int be_test_child(int argc, char **argv);

#endif /* DFG_TEST_SUPPORT_H */
