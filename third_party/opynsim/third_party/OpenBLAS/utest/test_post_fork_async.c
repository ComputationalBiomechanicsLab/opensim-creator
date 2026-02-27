/*****************************************************************************
Copyright (c) 2011-2025, The OpenBLAS Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. Neither the name of the OpenBLAS project nor the names of
      its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <cblas.h>
#include "openblas_utest.h"

static void* xmalloc(size_t n)
{
    void* tmp;
    tmp = calloc(1, n);
    if (tmp == NULL) {
        fprintf(stderr, "Failed to allocate memory for the testcase.\n");
        exit(1);
    } else {
        return tmp;
    }
}

CTEST(fork, safety_after_fork_async)
{
#ifdef __UCLIBC__
#if !defined __UCLIBC_HAS_STUBS__ && !defined __ARCH_USE_MMU__
exit(0);
#endif
#endif
#ifndef BUILD_DOUBLE
exit(0);
#else
    blasint n = 200;
    blasint info;
    blasint i;

    blasint *ipiv;
    double *arr;

    pid_t fork_pid;

    arr = xmalloc(sizeof(*arr) * n * n);
    ipiv = xmalloc(sizeof(*ipiv) * n);

    // array is an identity matrix
    for (i = 0; i < n*n; i += n + 1) {
        arr[i] = 1.0;
    }

    fork_pid = fork();
    if (fork_pid == -1) {
        perror("fork");
        CTEST_ERR("Failed to fork process.");
    } else if (fork_pid == 0) {
      exit(0);
    } else {
        // Wait for the child to finish and check the exit code.
        int child_status = 0;
        pid_t wait_pid = wait(&child_status);
        ASSERT_EQUAL(wait_pid, fork_pid);
        ASSERT_EQUAL(0, WEXITSTATUS (child_status));
    }
    BLASFUNC(dgetrf)(&n, &n, arr, &n, ipiv, &info);
#endif
}
