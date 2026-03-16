#!/usr/bin/env bash

# Enable nullglob so *.exe expands to nothing if no matches
shopt -s nullglob

SEARCH_DIR="${1:-.}"

# Resolve to absolute path to avoid relative path issues when tools are called
if command -v realpath >/dev/null 2>&1; then
    SEARCH_DIR=$(realpath "$SEARCH_DIR")
elif command -v readlink >/dev/null 2>&1; then
    SEARCH_DIR=$(readlink -f "$SEARCH_DIR")
fi

if [[ ! -d "$SEARCH_DIR" ]]; then
    echo "Error: Directory '$SEARCH_DIR' does not exist."
    exit 1
fi

echo "Searching for .exe files in '$SEARCH_DIR'..."

# Move to the search directory so all relative paths and extractions are internal
cd "$SEARCH_DIR" || exit 1

for file_path in *.exe; do
    file=$(basename "$file_path")
    # Remove .exe extension
    folder="${file%.exe}"

    echo "Processing '$file'..."

    if [[ "$file" == REN_* ]] || [[ "$file" == "h8v7000_ev.exe" ]]; then
        echo "Using 7zip for '$file'..."
        mkdir -p "$folder"
        if ! 7zz x "$file" -o"$folder"; then
            echo "Error: 7zip extraction failed for '$file'."
            continue
        fi
    elif [[ "$file" == h8v6* ]]; then
        echo "Using ISx for '$file'..."
        folder="${file}_u"
        if ! isx "$file"; then
            echo "Error: Extraction failed for '$file' using ISx."
            continue
        fi
    else
        echo "Skipping '$file': No matching extraction rule."
        continue
    fi

    # After unpacking the executable, look for data1.cab in the resulting folder (case-insensitive)
    echo "Searching for data1.cab in '$folder'..."
    find "$folder" -iname "data1.cab" | while read -r cab_file; do
        cab_dir=$(dirname "$cab_file")
        cab_name=$(basename "$cab_file")
        echo "Checking file groups in '$cab_file'..."
        
        # Get list of groups
        groups=$(unshield g "$cab_file")
        
        target_group=""
        if echo "$groups" | grep -q "^E_Toolchains$"; then
            target_group="E_Toolchains"
        elif echo "$groups" | grep -q "^Toolchains$"; then
            target_group="Toolchains"
        elif echo "$groups" | grep -q "^C_Toolchains_H8$"; then
            target_group="C_Toolchains_H8"
        fi

        if [[ -n "$target_group" ]]; then
            echo "Unpacking group '$target_group' from '$cab_file' using unshield in '$cab_dir'..."
            (cd "$cab_dir" && unshield -g "$target_group" x "$cab_name")
            
            # Merge H8 folders to root
            echo "Merging H8 folders from '$cab_dir/$target_group/H8' to '/work/H8'..."
            mkdir -p "/work/H8"
            if [[ -d "$cab_dir/$target_group/H8" ]]; then
                # Move each version folder
                for version_dir in "$cab_dir/$target_group/H8"/*; do
                    if [[ -d "$version_dir" ]]; then
                        version_name=$(basename "$version_dir")
                        echo "Moving version '$version_name' to /work/H8/..."
                        mv "$version_dir" "/work/H8/"
                    fi
                done
            fi
        else
            echo "Error: Neither 'E_Toolchains' nor 'Toolchains' group found in '$cab_file'."
            echo "Available groups:"
            echo "$groups"
        fi
    done

    echo "Successfully processed '$file'"
    echo "--------------------------"
done

echo "All extractions complete."