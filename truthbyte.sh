#!/bin/bash

# =========================================================================
# TruthByte: A CLI tool to verify file integrity using hash algorithms
# 
# This script verifies file or directory contents by comparing computed 
# hashes against expected values. It supports multiple hash algorithms
# and can process both individual files and directories.
#
# Author: Salman
# Version: 2.0-1
# =========================================================================

readonly VERSION="2.0-1"

# === Color Vars for console output formatting ===
readonly RED='\033[0;31m'    # Used for errors and mismatches
readonly GREEN='\033[0;32m'  # Used for successful matches
readonly YELLOW='\033[0;33m' # Used for warnings
readonly BLUE='\033[0;34m'   # Used for information headers
readonly NC='\033[0m'        # No Color - resets formatting

# === Utility Functions ===
expand_path() {
    # Expands tilde (~) to home directory path
    # Params:
    #   $1: Input path that may contain tilde
    # Returns:
    #   Expanded path with ~ replaced by $HOME
    local input="$1"
    [[ "$input" =~ ^~ ]] && echo "${input/#\~/$HOME}" || echo "$input"
}

check_command() {
    # Checks if a required command exists on the system
    # Params:
    #   $1: Command to check
    # Exits with error if command not found
    command -v "$1" &>/dev/null || {
        echo -e "${RED}Missing dependency: $1${NC}"
        exit 1
    }
}

# Validate Bash version - associative arrays require Bash 4.0+
if ((BASH_VERSINFO[0] < 4)); then
    echo "This script requires Bash version 4.0 or higher"
    exit 1
fi

error_exit() {
    # Prints an error message and exits the script
    # Params:
    #   $1: Error message to display
    # Also logs to output file if specified
    echo -e "${RED}Error: $1${NC}" >&2
    [ -n "$OUTPUT_FILE" ] && echo "Error: $1" >> "$OUTPUT_FILE"
    exit 1
}

log_message() {
    # Outputs a message to console and optionally to log file
    # Params:
    #   $1: Message to display/log
    # Strips ANSI color codes when writing to file
    echo -e "$1"
    [ -n "$OUTPUT_FILE" ] && echo -e "$1" | sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,3})*)?[mGK]//g" >> "$OUTPUT_FILE"
}

show_help() {
    # Displays help information with usage examples
    # No parameters
    # Exits after displaying help
    cat <<EOF
TruthByte $VERSION - Verify file integrity using hash algorithms

Usage:
  truthbyte [options] <path> [hash]
  
  For file verification:
    truthbyte -a <algorithm> <file_path> <expected_hash>
  
  For directory verification:
    truthbyte -a <algorithm> [-r|--recursive] [-v|--verify-from <hash_file>] <directory_path>

Options:
  -a <algorithm>     : Hash algorithm (sha256, sha512, sha1, md5)
  -r, --recursive    : Recursively process directories
  -v, --verify-from <file>: Read expected hashes from file (for directories)
  -o, --output <file>: Output verification results to a text file (.txt extension required)
  -h, --help         : Show this help message
  --version          : Show version
  --upgrade          : Upgrade TruthByte CLI (requires Git repo)

Examples:
  truthbyte -a sha256 ~/file.iso abc123def456...
  truthbyte -a md5 file.zip 9a0364b9e99bb480dd25e1f0284c8555
  truthbyte -a sha256 -v hashes.sha256 ~/my_files/
  truthbyte -a sha256 -r --verify-from hashes.md5 -o results.txt ~/my_files/
EOF
    exit 0
}

show_version() {
    # Displays the current version of the script
    # No parameters
    # Exits after displaying version
    echo "TruthByte version $VERSION"
    exit 0
}

upgrade_tool() {
    # Placeholder for future upgrade functionality
    # Would download and install the latest version
    echo "Upgrade functionality not implemented yet"
    exit 0
}

# === Process hash file and verify files ===
process_directory() {
    # Main function for directory-based hash verification
    # Params:
    #   $1: Directory path to process
    #   $2: Hash file containing expected hashes
    #   $3: Boolean flag for recursive processing
    #   $4: Hash command to use (sha256sum, md5sum, etc.)
    # Returns:
    #   0: All checked files matched
    #   1: Some files had hash mismatches
    #   2: No files were checked
    local dir="$1"
    local hash_file="$2"
    local is_recursive="$3"
    local sum_cmd="$4"
    
    log_message "${BLUE}=== Starting file verification ===${NC}"
    log_message "Directory: $dir"
    log_message "Hash file: $hash_file"
    log_message "Algorithm: ${HASH_TYPE}"
    log_message "Recursive: $is_recursive"
    
    # Validate hash file exists
    [ ! -f "$hash_file" ] && error_exit "Hash file does not exist: $hash_file"
    
    # Initialize counters for tracking verification results
    local total_files=0
    local checked_files=0
    local matched_files=0
    local mismatched_files=0
    local notfound_files=0
    
    # Create a lookup map of files in the directory for faster searching
    log_message "\n${BLUE}Scanning directory...${NC}"
    # Declare associative array for file mapping
    declare -A file_map
    
    # Ensure directory path ends with a slash for consistent path handling
    dir="${dir%/}/"
    
    # Scan directory and populate file map based on recursive flag
    if [ "$is_recursive" = true ]; then
        # For recursive mode, scan all files and store both relative paths and basenames
        while IFS= read -r file; do
            # Store relative path for comparison with hash file
            local rel_path="${file#$dir}"
            file_map["$rel_path"]="$file"
            
            # Also store the basename for files that may be referenced directly
            local base_name=$(basename "$file")
            file_map["$base_name"]="$file"
            
            ((total_files++))
        done < <(find "$dir" -type f | sort)
    else
        # For non-recursive mode, only scan files in the top directory
        while IFS= read -r file; do
            local rel_path="$(basename "$file")"
            file_map["$rel_path"]="$file"
            ((total_files++))
        done < <(find "$dir" -maxdepth 1 -type f | sort)
    fi
    
    log_message "Found $total_files files in directory"
    
    # Process the hash file
    log_message "\n${BLUE}Processing hash file...${NC}"
    local line_count=0
    
    # Read each line from the hash file
    while IFS= read -r line; do
        ((line_count++))
        
        # Skip empty lines and comments
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        
        # Extract hash and filename
        # Format: <hash>  <filename> or <filename>  <hash>
        local hash filename
        
        # Try to parse the line - support both standard formats
        if [[ "$line" =~ ^([[:xdigit:]]+)[[:space:]]+(.+)$ ]]; then
            # Format: <hash> <filename>
            hash="${BASH_REMATCH[1]}"
            filename="${BASH_REMATCH[2]}"
            # Trim whitespace from filename
            filename=$(echo "$filename" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        elif [[ "$line" =~ ^(.+)[[:space:]]+([[:xdigit:]]+)$ ]]; then
            # Format: <filename> <hash>
            filename="${BASH_REMATCH[1]}"
            hash="${BASH_REMATCH[2]}"
            # Trim whitespace from filename
            filename=$(echo "$filename" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        else
            # Invalid format - log warning and skip
            log_message "${YELLOW}Warning: Invalid format in line $line_count: $line${NC}"
            continue
        fi
        
        log_message "\n${BLUE}Verifying:${NC} $filename"
        
        # Check if the file exists in our directory using the file map
        local full_path="${file_map[$filename]}"
        
        # If not found directly, try with path adjustments (for recursive mode)
        if [ -z "$full_path" ] && [ "$is_recursive" = true ]; then
            # Try with alternative path separators (Windows/Unix compatibility)
            local alt_filename="${filename//\\//}"  # Replace backslashes with forward slashes
            full_path="${file_map[$alt_filename]}"
            
            # If still not found, try to find by basename as a fallback
            if [ -z "$full_path" ]; then
                local base_name=$(basename "$filename")
                log_message "${YELLOW}Trying to match by basename: $base_name${NC}"
                full_path="${file_map[$base_name]}"
            fi
        fi
        
        # If file was found, verify its hash
        if [ -n "$full_path" ] && [ -f "$full_path" ]; then
            # Calculate the hash of the actual file
            local actual_hash=$($sum_cmd "$full_path" | awk '{print $1}')
            
            log_message "Expected hash: $hash"
            log_message "Actual hash  : $actual_hash"
            
            # Compare the hashes
            if [[ "$actual_hash" == "$hash" ]]; then
                log_message "${GREEN}✓ Match${NC}"
                ((matched_files++))
            else
                log_message "${RED}✗ Mismatch${NC}"
                ((mismatched_files++))
            fi
            
            ((checked_files++))
        else
            # File not found - log and count
            log_message "${YELLOW}File not found in directory: $filename${NC}"
            ((notfound_files++))
        fi
    done < "$hash_file"
    
    # Print summary of verification results
    log_message "\n${BLUE}=== Verification Summary ===${NC}"
    log_message "Total files in directory: $total_files"
    log_message "Files checked: $checked_files"
    log_message "Matches: ${GREEN}$matched_files${NC}"
    log_message "Mismatches: ${RED}$mismatched_files${NC}"
    log_message "Files not found: ${YELLOW}$notfound_files${NC}"
    
    # Determine the exit status based on verification results
    if [ $mismatched_files -eq 0 ] && [ $checked_files -gt 0 ]; then
        log_message "\n${GREEN}All checked files verified successfully!${NC}"
        return 0
    elif [ $checked_files -eq 0 ]; then
        log_message "\n${YELLOW}Warning: No files were checked!${NC}"
        return 2
    else

        log_message "\n${RED}Some files failed verification!${NC}"
        return 1
    fi
}

# === Setup interrupt handler ===
# Catch Ctrl+C and other termination signals
trap 'echo -e "${RED}Script interrupted.${NC}"; exit 1' INT TERM

# === Initialize global variables ===
RECURSIVE=false
HASH_FILE=""
TARGET_PATH=""
EXPECTED_HASH=""
HASH_TYPE=""
OUTPUT_FILE=""

# === Parse CLI arguments ===
# Uses indexed loop for flexible argument order
i=1
while [ $i -le $# ]; do
    arg="${!i}"
    case "$arg" in
        -h|--help)
            # Show help and exit
            show_help
            ;;
        --version)
            # Show version and exit
            show_version
            ;;
        --upgrade)
            # Run upgrade tool and exit
            upgrade_tool
            ;;
        -r|--recursive)
            # Enable recursive directory processing
            RECURSIVE=true
            ;;
        -v|--verify-from)
            # Get hash file path from next argument
            i=$((i+1))
            if [ $i -le $# ] && [[ ! "${!i}" =~ ^- ]]; then
                HASH_FILE="${!i}"
            else
                error_exit "Option -v/--verify-from requires a hash file argument"
            fi
            ;;
        -o|--output)
            # Get output file path from next argument
            i=$((i+1))
            if [ $i -le $# ] && [[ ! "${!i}" =~ ^- ]]; then
                OUTPUT_FILE="${!i}"
                # Validate output file has .txt extension
                if [[ ! "$OUTPUT_FILE" =~ \.txt$ ]]; then
                    error_exit "Output file must have a .txt extension"
                fi
            else
                error_exit "Option -o/--output requires a file argument"
            fi
            ;;
        -a)
            # Get hash algorithm from next argument
            i=$((i+1))
            if [ $i -le $# ] && [[ ! "${!i}" =~ ^- ]]; then
                HASH_TYPE="${!i}"
            else
                error_exit "Option -a requires an algorithm argument"
            fi
            ;;
        -*)
            # Handle invalid options
            error_exit "Invalid option: $arg"
            ;;
        *)
            # Handle positional arguments (path and hash)
            if [ -z "$TARGET_PATH" ]; then
                TARGET_PATH=$(expand_path "$arg")
            elif [ -z "$EXPECTED_HASH" ]; then
                EXPECTED_HASH=$(echo "$arg" | tr -d '[:space:]')
            else
                error_exit "Too many arguments: $arg"
            fi
            ;;
    esac
    i=$((i+1))
done

# === Validate required input parameters ===
[ -z "$TARGET_PATH" ] && error_exit "No path provided"
[ -z "$HASH_TYPE" ] && error_exit "No hash algorithm provided (-a)"

# === Initialize output file if specified ===
if [ -n "$OUTPUT_FILE" ]; then
    # Create or clear the output file and add header
    echo "TruthByte Verification Log - $(date)" > "$OUTPUT_FILE"
    echo "Version: $VERSION" >> "$OUTPUT_FILE"
    echo "=========================" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
fi

# === Set up the hash command based on selected algorithm ===
case $HASH_TYPE in
    sha256) SUM_CMD="sha256sum" ;;
    sha1)   SUM_CMD="sha1sum" ;;
    sha512) SUM_CMD="sha512sum" ;;
    md5)    SUM_CMD="md5sum" ;;
    *) error_exit "Unsupported hash algorithm: $HASH_TYPE" ;;
esac

# Verify the hash command exists on the system
check_command "$SUM_CMD"

# === Main execution logic based on target type ===
if [ -d "$TARGET_PATH" ]; then
    # Directory mode - verify files against hash file
    [ -z "$HASH_FILE" ] && error_exit "Directory mode requires -v/--verify-from <hash_file>"
    process_directory "$TARGET_PATH" "$HASH_FILE" "$RECURSIVE" "$SUM_CMD"
    exit $?
elif [ -f "$TARGET_PATH" ]; then
    # Single file mode - verify against provided hash
    [ -z "$EXPECTED_HASH" ] && error_exit "Single file mode requires a hash argument"
    
    # Calculate and verify the hash
    ACTUAL_HASH=$($SUM_CMD "$TARGET_PATH" | awk '{print $1}')
    if [[ "$ACTUAL_HASH" == "$EXPECTED_HASH" ]]; then
        log_message "${GREEN}Success: Hash matched!${NC}"
        exit 0
    else
        log_message "${RED}Mismatch: Hashes do not match!${NC}"
        log_message "Expected: $EXPECTED_HASH"
        log_message "Actual  : $ACTUAL_HASH"
        exit 1
    fi
else
    # Target path doesn't exist or isn't accessible
    error_exit "Path does not exist or is not accessible: $TARGET_PATH"
fi
