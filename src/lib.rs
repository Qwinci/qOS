use proc_macro::TokenStream;

#[proc_macro]
pub fn replace(items: TokenStream) -> TokenStream {
    let mut items_str = items.to_string();

    let matches: Vec<_> = items_str.match_indices("[<").map(|(index, _)| index)
        .zip(items_str.match_indices(">]").map(|(index, _)| index)).collect();

    for (start, end) in matches {
        let mut replace = items_str[start + 2..end].to_string();
        replace.retain(|c| !c.is_whitespace());
        items_str.replace_range(start..end + 2, &replace);
    }

    items_str.parse().unwrap()
}