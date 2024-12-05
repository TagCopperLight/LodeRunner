\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{Template}[2024/11/20 v1.0 Template]

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{report}}

\ProcessOptions\relax

%%% Load Master Class %%%
\LoadClass[a4paper,twoside,11pt]{report}


%%% Required Packages (With Additional Information) %%%
\usepackage{graphicx} % Required for including images.
\usepackage{amsmath} % Mathematical expressions.
\usepackage{amssymb} % Mathematical symbols.
\usepackage[dvipsnames]{xcolor} % Using colors within the document.
\usepackage[toc,page]{appendix} % Add appendix.
\usepackage{titling} % Titlepage and titleback customisation.
\usepackage[calc,warn=false]{datetime2} % Current date with both.
\usepackage{tcolorbox} % Highlighted box.
\usepackage{subcaption} % For side-by-side figures.
\usepackage{xparse} % "Davide" problem... solved!
\usepackage{ifthen} % Conditional statements.
\usepackage{silence} % No warnings in certain packages.
\usepackage{typearea} % KOMA options for page rotation.
\usepackage{eso-pic} % For the background in both front and back matter.
\usepackage{setspace} % For line height adjusts.
\usepackage{varwidth} % Dependency for the custom boxes.
\usepackage{etoolbox} % Toolbox of programming tools for LaTeX classes.
\usepackage{blindtext} % Generate dummy text.

\tcbuselibrary{most} % Load more libaries from tcolorbox package.
\WarningsOff[blindtext] % No support for portuguese language. Silence: On.

%%% Import of Configurations -- Do Not Change the Order! %%%
\input{Configurations/00-Fonts}
\input{Configurations/01-Margins}
\input{Configurations/02-Bibliography}
\input{Configurations/03-Header-Footer}
\input{Configurations/04-ToC}
\input{Configurations/05-Glossary}
\input{Configurations/06-Chapters}
\input{Configurations/07-Tables-Captions}
\input{Configurations/08-Code}
\input{Configurations/09-Custom-Commands}
\input{Configurations/10-Miscellaneous}
\input{Configurations/11-Variables}