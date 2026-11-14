// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import static org.junit.jupiter.api.Assertions.*;

import org.junit.jupiter.api.Test;

/** The checks that enable Run/Debug for a C source (main function defined). */
class RunnableSourceTest {

    @Test
    void definitionIsFoundDeclarationAndCallAreNot() {
        assertTrue(IDEApp.definesFunction("int main(void) { return 0; }", "main"));
        assertTrue(IDEApp.definesFunction("int main(int argc, char **argv)\n{\n}", "main"));
        assertTrue(IDEApp.definesFunction("int main(int n, int (*f)(int)) {}", "main"));
        assertFalse(IDEApp.definesFunction("int main(void);", "main"));
        assertFalse(IDEApp.definesFunction("int x = domain(1);", "main"));
        assertFalse(IDEApp.definesFunction("void f(void) { s.main(1); p->main(2); }", "main"));
        assertTrue(IDEApp.definesFunction("int start(void) { return 0; }", "start"), "configurable name");
    }

    @Test
    void commentsAndStringsAreIgnored() {
        assertFalse(IDEApp.definesFunction("// int main(void) {\n", "main"));
        assertFalse(IDEApp.definesFunction("/* int main(void) {\n} */", "main"));
        assertFalse(IDEApp.definesFunction("const char *s = \"int main(void) {\";", "main"));
        assertTrue(IDEApp.definesFunction("/* x */ int main(void) /* y */ { }", "main"));
    }

    @Test
    void stripCommentsKeepsLines() {
        String s = IDEApp.stripComments("a // c\nb /* x\ny */ c\n", false);
        assertEquals(4, s.split("\n", -1).length);
        assertTrue(s.startsWith("a "));
        assertTrue(s.contains("c"));
    }
}
