[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-2e0aaae1b6195c2367325f4f02e2d04e9abb55f0b24a779b69b11b9e10269abc.svg)](https://classroom.github.com/online_ide?assignment_repo_id=20630500&assignment_repo_type=AssignmentRepo)

# Title: An awesome currency exchange program

## Members/Git accounts

- Günther Miklas / gumikl
- Klaus Jesper Zaletajev /
- Joosep Rehepapp / papi-png
- Mathias Siimon / Matuffi

## Description:

This is our collective work of course ICS0017 project.

## Build & Run

- TBA

## Release workflow

- We keep ONE repository for the whole project.
- At the end of each checkpoint, mark a release tag and add your report.

## Mark a release tag

### end of Release 1:

git tag release-1
git push origin release-1

### end of Release 2:

git tag release-2
git push origin release-2

### Folders for documents

docs/release-1/  # slides, SRS/SDP updates, test report

docs/release-2/

docs/release-3/

## Workflow overview:

**Pull latest changes → Stay up to date with teammates.**

- git checkout main
- git pull origin main

**Create a branch → Work on a feature/bugfix without touching main.**

- git checkout -b feature/my-feature

**Commit changes → Save progress locally.**

- git status                       # see changes
- git add file1 file2              # stage specific files
- git add .                        # stage everything
- git commit -m "Add new feature"  # commit with a clear message

**Push branch to remote → Upload your work.**

- git push origin feature/my-feature

**Open a Pull Request (PR) → Request teammates to review/merge.**

1. Go to repo on GitHub.
2. You’ll see “Compare & pull request” after pushing a new branch.
3. Fill in:
   - Title: short description
   - Body: what you changed, why, any notes
4. Submit the PR → teammates can:
   - Review code
   - Suggest changes
   - Approve & merge
5. Merge into main → After approval.
