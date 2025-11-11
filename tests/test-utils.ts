import { execSync } from 'child_process';
import { writeFileSync, unlinkSync } from 'fs';
import { tmpdir } from 'os';
import { join } from 'path';

/**
 * Extracts the program output from the full technoscript execution output.
 * Looks for content after "=== Executing Generated Code ===" until "=== Execution Complete ===".
 */
export function extractProgramOutput(fullOutput: string): string {
  const startMarker = '=== Executing Generated Code ===';
  const endMarker = '=== Execution Complete';

  const startIndex = fullOutput.indexOf(startMarker);
  const endIndex = fullOutput.indexOf(endMarker);

  if (startIndex === -1 || endIndex === -1 || endIndex <= startIndex) {
    return '';
  }

  // Extract the content after the start marker until the end marker
  const startOfContent = startIndex + startMarker.length;
  return fullOutput.substring(startOfContent, endIndex).trim();
}

/**
 * Creates a temporary file path for test programs.
 */
export function createTempFilePath(filename: string = 'test_program.ts'): string {
  return join(tmpdir(), filename);
}

/**
 * Helper to clean up temporary files after tests.
 */
export function cleanupTempFile(filePath: string): void {
  try {
    unlinkSync(filePath);
  } catch (e) {
    // ignore
  }
}

/**
 * Runs a technoscript program and returns the full output.
 */
export function runTechnoscriptProgram(program: string, tempFile: string): string {
  writeFileSync(tempFile, program);
  return execSync(`${process.cwd()}/../technoscript ${tempFile}`, { encoding: 'utf8' });
}
