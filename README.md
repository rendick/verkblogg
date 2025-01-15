# verkblogg

verkblogg is a utility for optimizing blogging

## Installation

```
soon
```

## Usage

Beforehand, you should create a folder with the files `public/index.html`, `public/article.html` and `public/template.rss`. As an example, look at [these public files](https://github.com/rendick/verkblogg/tree/main/public). You can change the HTML and RSS content, but without deleting the text ${articleTitle}, ${main}, ${title} and ${items}).

The main functionality consists of running `verkblogg create`, where the user will enter information (title and main content text) that will later be sent to the `verkblogg.db` file in the format "Title;body text;path". The code will then parse `verkblogg.db` (DB of all posts) and put the information into `index.html` and articles/{path}.html, thus creating/updating them.

For more information about all features, use the `verkblogg help`

## What are the advantages?

Firstly, it will simplify the creation of posts, since the utility is responsible for creating/updating folders, HTML files and DBs (Saves time and effort). Secondly, due to the fact that all posts are stored in one file, you can save and move it without any problems. Third, the site can be hosted on static hostings (e.g. Github Pages).
