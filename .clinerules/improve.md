This is a typescript + additions compiler.

Your job is to continue to intelligently implement features in the parser/ast at parser/ into the compiler.

Once you finish a feature, you need to create a test for it in tests/  These are simple tests that compare the input to the expected output, which you have to extract with extractProgramOutput, as there is a lot of debug output right now as well. You use runTechnoscriptProgram to run it. There are helpers in tests/test-utils.ts to make life easier. Look at an existing test for example.

Your job is to continue implementing features and check off unimplemented_features.md when each feature is complete. A completed feature should have an associated bun test.

The goal is to make a high performance, multi core compatible system.

We already have gc and basic goroutines, but we were using an old parser. So you may run into any number of problems. If you do, you can update the parser/ast with new nodes or properties as needed, but try to avoid changing it's structure.

Your goal is to make this as high performance as possible, so we are emitting raw assembly, essentially.