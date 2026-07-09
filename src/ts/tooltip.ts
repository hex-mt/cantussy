const TOOLTIP_GAP_PX = 12;
const TOOLTIP_POINT_SIZE_PX = 6;

function createPoint(): HTMLDivElement {
    const point = document.createElement("div");
    point.className =
        "absolute top-full left-1/2 border-t-stone-700 dark:border-t-orange-100";
    point.style.marginLeft = `-${TOOLTIP_POINT_SIZE_PX}px`;
    point.style.borderLeft = `${TOOLTIP_POINT_SIZE_PX}px solid transparent`;
    point.style.borderRight = `${TOOLTIP_POINT_SIZE_PX}px solid transparent`;
    point.style.borderTopWidth = `${TOOLTIP_POINT_SIZE_PX}px`;
    point.style.borderTopStyle = "solid";
    return point;
}

export interface TooltipHandle {
    // Overrides the currently-shown tooltip's text (e.g. to confirm an
    // action the hovered element just performed). Reverts to the original
    // text next time the tooltip is hidden and re-shown. No-ops if the
    // tooltip isn't currently visible.
    flash(text: string): void;
}

export function attachTooltip(el: Element, text: string): TooltipHandle {
    // Wrapped separately from `el` so the tooltip isn't caught up in
    // `el`'s own hover/active transform (e.g. hover:scale-110), and so it
    // stays anchored in normal document flow rather than viewport-fixed.
    const wrapper = document.createElement("span");
    wrapper.className = "relative inline-block";
    el.replaceWith(wrapper);
    wrapper.appendChild(el);

    let tooltip: HTMLDivElement | null = null;
    let textNode: Text | null = null;

    function show() {
        tooltip = document.createElement("div");
        tooltip.className =
            "absolute z-50 bottom-full left-1/2 -translate-x-1/2 px-3 py-1.5 rounded text-md whitespace-nowrap pointer-events-none " +
            "bg-stone-700 text-orange-100 dark:bg-orange-100 dark:text-stone-800";
        tooltip.style.marginBottom = `${TOOLTIP_GAP_PX + TOOLTIP_POINT_SIZE_PX}px`;
        textNode = document.createTextNode(text);
        tooltip.appendChild(textNode);
        tooltip.appendChild(createPoint());
        wrapper.appendChild(tooltip);
    }

    function hide() {
        tooltip?.remove();
        tooltip = null;
        textNode = null;
    }

    el.addEventListener("mouseenter", show);
    el.addEventListener("mouseleave", hide);

    return {
        flash(flashText: string) {
            if (textNode) textNode.data = flashText;
        },
    };
}
