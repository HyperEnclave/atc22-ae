from typing import List


class Variant:
    def __init__(self, label, data, color=None, marker=None):
        self.label = label
        self.data = data
        self.color = color
        self.marker = marker

    def normalize(baseline: 'Variant', variants: List['Variant']) -> List['Variant']:
        new_variants = []
        for v in variants:
            new_data = [b / a for (a, b) in zip(baseline.data, v.data)]
            new_variants.append(Variant(v.label, new_data, v.color, v.marker))
            # print("Normalized:", new_variants[-1])
        return new_variants

    def __str__(self):
        return "Variant(\'%s\", %s)" % (self.label, self.data)

    def __repr__(self):
        return str(self)
