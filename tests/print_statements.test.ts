import { writeFileSync } from 'fs';
import { tmpdir } from 'os';
import { join } from 'path';
import { extractProgramOutput, runTechnoscriptProgram, cleanupTempFile } from './test-utils';

describe('Print Statements', () => {
  const tempFile = join(tmpdir(), 'test_program.ts');

  afterEach(() => {
    cleanupTempFile(tempFile);
  });

  it('should output the value when printing a variable', () => {
    const program = 'var x: int64 = 42; print(x);';
    const output = runTechnoscriptProgram(program, tempFile);
    const programOutput = extractProgramOutput(output);

    // Should output "42" (the value of x)
    expect(programOutput).toBe('42');
  });
});
