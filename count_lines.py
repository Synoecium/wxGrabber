#!/usr/bin/env python3
"""
Script to count lines of code in the wxGrabber project
Counts all C/C++ source and header files, excluding build directories, git files, and minivorbis library
"""

import os
import sys
from pathlib import Path
from collections import defaultdict
from typing import List, Tuple, Dict

def should_exclude_file(file_path: Path) -> bool:
    """Check if a file should be excluded from counting"""
    path_str = str(file_path)
    
    # Exclude patterns
    exclude_patterns = [
        '.git',
        'build',
        'CMakeFiles',
        'minivorbis'
    ]
    
    for pattern in exclude_patterns:
        if pattern in path_str:
            return True
    
    return False

def count_lines_in_file(file_path: Path) -> int:
    """Count lines in a file"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return len(f.readlines())
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}")
        return 0

def get_relative_path(file_path: Path, base_path: Path) -> str:
    """Get relative path from base directory"""
    try:
        return str(file_path.relative_to(base_path))
    except ValueError:
        return str(file_path)

def main():
    print("Counting lines of code in wxGrabber project...")
    print("=" * 60)
    
    # Get current directory
    base_path = Path.cwd()
    
    # File extensions to include
    source_extensions = {'.cpp', '.c'}
    header_extensions = {'.h', '.hpp'}
    config_extensions = {'.txt', '.cmake', '.json', '.cfg'}
    
    # Find all relevant files
    all_extensions = source_extensions | header_extensions
    all_files = []
    
    for ext in all_extensions:
        pattern = f"**/*{ext}"
        files = list(base_path.glob(pattern))
        all_files.extend(files)
    
    # Filter out excluded files
    filtered_files = [f for f in all_files if not should_exclude_file(f)]
    
    # Separate source and header files
    source_files = []
    header_files = []
    
    for file_path in filtered_files:
        line_count = count_lines_in_file(file_path)
        relative_path = get_relative_path(file_path, base_path)
        
        file_info = {
            'path': relative_path,
            'lines': line_count,
            'extension': file_path.suffix
        }
        
        if file_path.suffix in source_extensions:
            source_files.append(file_info)
        elif file_path.suffix in header_extensions:
            header_files.append(file_info)
    
    # Sort files by path
    source_files.sort(key=lambda x: x['path'])
    header_files.sort(key=lambda x: x['path'])
    
    # Display source files
    print("\nSource Files (.cpp, .c):")
    print("-" * 40)
    source_total = 0
    for file_info in source_files:
        print(f"{file_info['path']:<40} {file_info['lines']:>6} lines")
        source_total += file_info['lines']
    
    # Display header files
    print("\nHeader Files (.h, .hpp):")
    print("-" * 40)
    header_total = 0
    for file_info in header_files:
        print(f"{file_info['path']:<40} {file_info['lines']:>6} lines")
        header_total += file_info['lines']
    
    # Summary
    total_files = len(source_files) + len(header_files)
    total_lines = source_total + header_total
    
    print("\n" + "=" * 60)
    print("SUMMARY:")
    print("=" * 60)
    print(f"Source Files ({len(source_files)}): {source_total:>8} lines")
    print(f"Header Files ({len(header_files)}): {header_total:>8} lines")
    print("-" * 40)
    print(f"\033[42mTotal Files  ({total_files}): {total_lines:>8} lines\033[0m")
    
    # Largest files
    all_files_info = source_files + header_files
    largest_files = sorted(all_files_info, key=lambda x: x['lines'], reverse=True)[:5]
    
    print("\nLargest files:")
    for file_info in largest_files:
        print(f"{file_info['path']:<40} {file_info['lines']:>6} lines")
    
    # File breakdown by extension
    print("\nFile breakdown by extension:")
    extension_stats = defaultdict(int)
    for file_info in all_files_info:
        extension_stats[file_info['extension']] += file_info['lines']
    
    for ext in sorted(extension_stats.keys()):
        print(f"{ext:<5} files: {extension_stats[ext]:>6} lines")
    
    # Configuration files
    print("\nConfiguration Files:")
    print("-" * 40)
    
    config_files = []
    config_patterns = ['CMakeLists.txt', '*.cmake', '*.json', '*.cfg']
    
    for pattern in config_patterns:
        files = list(base_path.glob(pattern))
        for f in files:
            if not should_exclude_file(f):
                config_files.append(f)
    
    config_total = 0
    for file_path in config_files:
        line_count = count_lines_in_file(file_path)
        relative_path = get_relative_path(file_path, base_path)
        print(f"{relative_path:<40} {line_count:>6} lines")
        config_total += line_count
    
    if config_total > 0:
        print(f"\nConfiguration Total: {config_total:>8} lines")
        grand_total = total_lines + config_total
        print(f"\033[45mGrand Total (all): {grand_total:>10} lines\033[0m")
    
    # Additional statistics
    print(f"\nProject Statistics:")
    print(f"- Average lines per source file: {source_total/len(source_files):.1f}" if source_files else "- No source files")
    print(f"- Average lines per header file: {header_total/len(header_files):.1f}" if header_files else "- No header files")
    print(f"- Source to header ratio: {source_total/header_total:.2f}:1" if header_total > 0 else "- No header files for ratio")

if __name__ == "__main__":
    main() 