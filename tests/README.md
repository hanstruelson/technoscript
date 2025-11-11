# Technoscript Test Suite

This directory contains tests for the implemented features of the technoscript compiler.

## Running Tests

```bash
cd tests
bun install
bun test
```

## Test Structure

Tests use Bun's test framework and check actual program output by extracting the content between "=== Running program directly ===" and "=== Executing Generated Code ===" markers.

## Current Tests

- **Print Statements** (`print_statements.test.ts`): Tests that print statements actually output the expected values. Currently failing because print output is not implemented.

## Test Status

All tests currently fail as expected - they test for actual functionality that needs to be implemented. Once the print functionality is properly implemented to output between the execution markers, these tests will pass.
