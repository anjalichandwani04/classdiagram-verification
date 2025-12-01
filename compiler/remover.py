import os

# Folder where this script is located
base_folder = os.path.dirname(os.path.abspath(__file__))

# Go through all items inside the base folder
for item in os.listdir(base_folder):
    item_path = os.path.join(base_folder, item)

    # Only process directories (the 3 folders)
    if os.path.isdir(item_path):
        print(f"Cleaning folder: {item}")

        for filename in os.listdir(item_path):
            file_path = os.path.join(item_path, filename)

            # Skip directories inside the subfolder (if any)
            if os.path.isdir(file_path):
                continue

            # Delete all non-.puml files
            if not filename.lower().endswith(".puml"):
                try:
                    os.remove(file_path)
                    print(f"  Deleted: {filename}")
                except Exception as e:
                    print(f"  Error deleting {filename}: {e}")

print("All subfolders cleaned!")
