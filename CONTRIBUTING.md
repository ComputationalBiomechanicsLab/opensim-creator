# Contributing to OpenSim Creator

If you would like to contribute to OpenSim Creator then thank you 🥰: it's people like you
that make open-source awesome!

## Key Points

To maximize your chances of success, here's a few things to keep in mind when contributing to
OpenSim Creator:

1. Read the [Code of Conduct](CODE_OF_CONDUCT.md). Ensure you agree with it. It is a fairly
   standard document that can be summarized as "don't be a jerk".

2. Give the main developers a "heads up" by adding an issue to the [issues](issues)
   page, or emailing a developer. Write something that roughly describes what you intend to fix/change.
   This is very important. This lets us reach an agreement on your contribution before you put effort
   into it.

3. Make sure you can build the part of OpenSim Creator that you would like to contribute to.
   For example, if you plan on contributing to the documentation then you should probably
   build the documentation locally and ensure that your contribution works (exception:
   basic things like fixing typos, changing a string, etc. do not require doing this).

4. Make your changes, ensuring that you try to stick to the style of the existing code. Even if
   the style is disagreeable (which, in places, it is - but it's easier for us to fix that later
   if everything is in one style).

5. Ensure your contribution works on your local machine. Note: "works" does not
   necessarily imply "complete". For UI projects like OpenSim Creator, it is acceptable to
   submit working, but "incomplete", features if they provide something people need - provided
   the contribution doesn't destabilize other parts of the codebase.

6. Update `CHANGELOG.md` with an explanation of your change, if it is significant

7. Make pull requests (PRs) directly against the `main` branch. OpenSim Creator does not use
   separate branches for development, testing, prod, etc. Branches are typically deleted if they
   are old and no longer up-to-date with `main`.

8. If your change potentially (temporarily) breaks a feature in OpenSim Creator, or is extremely
   experimental, then you should hide it behind a flag, configuration option, UI checkbox,
   or similar. This is so that your change can be merged + shipped quickly without it "dangling
   around" on a branch for a large amount of time.
