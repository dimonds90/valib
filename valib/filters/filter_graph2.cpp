#include <algorithm>
#include "filter_graph2.h"

FilterGraph2::FilterGraph2()
{
  start.id = node_start;
  start.next = &end;
  start.prev = 0;
  start.filter = &pass_start;
  start.state = state_init;
  start.rebuild = no_rebuild;
  start.flushing = false;

  end.id = node_end;
  end.next = 0;
  end.prev = &start;
  end.filter = &pass_end;
  end.state = state_init;
  end.rebuild = no_rebuild;
  end.flushing = false;

  is_new_stream = false;
}

FilterGraph2::~FilterGraph2()
{
  Node *node = start.next;
  while (node->id != node_end)
  {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    Node *next_node = node->next;
    delete node;
    node = next_node;
  }
}

///////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
FilterGraph2::init(Speakers spk)
{
  destroy();
  start.filter->open(spk);
  return build_chain(&start);
}

void
FilterGraph2::uninit()
{
  destroy();
}

///////////////////////////////////////////////////////////////////////////////
// process_chain() process the data in the chain.
//
// It may leave the chain in 2 states:
// * empty: all nodes are in empty state. No data to return. Before calling
//   process_chunk() the next time we should fill the start node's input chunk
//   with next input chunk. To flush the chain, set flushing at the start node
//   to true.
//   process_chunk() returns false in this case.
//
// * processing: all nodes are in non-empty state. End node's output chunk
//   contains data to return.
//   process_chunk returns true in this case.
//
// Chain rebuild should be done only from the top of the chain. So, allow
// rebuild only when we reach start node. Also, we shouldn't rebuild the chain
// on flushing, because flushing may be initiated by a chain rebuild event
// upstream.
//
// Node processing algorithm:
//
//                              -----                  ----------
//         +----------------> ( GO UP ) <----------- (   Flush    )
//         |                    -----                ( Downstream )
//  +----- | -------------------> |                    ----------
//  |      |                      V                        ^
//  |      |             no   ---------   yes              |
//  |      |           +----< flushing? >----+     +------------------+
//  |      |           |      ---------      |     |     reset()      |
//  |      |           |                     |     | flushing = false |
//  |      |           V                     V     +------------------+
//  |      |      +---------+            +-------+         |
//  |      |      | Process |            | Flush |         |
//  |      |      +---------+            +-------+         |
//  |      |           |                     |             |
//  |      |           V                     |             |
//  |      |  +-----------------+            |             |
//  |      |  | Check the chain |            |             |
//  |      |  +-----------------+            |             |
//  |      |           |                     |             |
//  |      |           V                     V             |
//  |      |  no   ----------     yes    ----------   no   |
//  |      +-----< Have data? >----+---< Have data? >------+
//  |              ----------      |     ----------
//  |                              V
//  |   ----------           --------------
//  | (  Process   )  no   < New stream or  >
//  +-( Downsteram ) <---- < chain changed? >
//      ----------           --------------
//          ^                      | yes
//          |                      V
//          |                  ----------
//          |                (   Flush    )
//          |                ( Downstream )
//          |                  ----------
//          |                      |
//          |                      V
//          |              +---------------+
//          +--------------| Rebuild chain |
//                         +---------------+
//
///////////////////////////////////////////////////////////////////////////////

bool
FilterGraph2::chain_is_empty() const
{
  return start.state == state_init || start.state == state_empty;
}

bool
FilterGraph2::process_chain(Chunk2 &out)
{
  bool allow_chain_rebuild = false;

  // When chain is empty we should start processing from
  // the beginning of the chain and from the end otherwise.
  Node *node = chain_is_empty()? &start: &end;

  // Drop new stream state when we already sent it.
  if (!chain_is_empty()) is_new_stream = false;

  while (node)
  {
    ///////////////////////////////////////////////////////
    // Allow chain rebuilding when we reach start node

    if (node->id == node_start)
      allow_chain_rebuild = true;

    ///////////////////////////////////////////////////////
    // The last node requires special processing

    if (node->id == node_end)
    {
      if (node->flushing)
      {
        node->state = state_init;
        node->flushing = false;
        node = node->prev;
        continue;
      }

      if (!node->filter->process(node->input, node->output))
      {
        node->state = state_empty;
        node = node->prev;
        continue;
      }

      // Exit chain processing
      node->state = state_processing;
      out = node->output;
      return true;
    }

    ///////////////////////////////////////////////////////
    // Node processing
    ///////////////////////////////////////////////////////

    switch (node->state)
    {
    case state_init:
    case state_empty:
    case state_processing:
      /////////////////////////////////////////////////////
      // Process data

      if (node->flushing)
      {
        if (!node->filter->flush(node->output))
        {
          // done flushing this node
          node->filter->reset();
          node->flushing = false;
          if (node->next->state == state_init)
          {
            // no need to flush downstream, go up
            node->state = state_init;
            node = node->prev;
          }
          else
          {
            // flush downstream
            node->state = state_done_flushing;
            node->next->flushing = true;
            node = node->next;
          }
          continue;
        }
      }
      else
      {
        if (!node->filter->process(node->input, node->output))
        {
          // no data, go up to get more
          node->state = state_empty;
          node = node->prev;
          continue;
        }

        // check the chain
        if (allow_chain_rebuild && node->rebuild == check_rebuild)
        {
          if (node->next->id != next_id(node->id, node->filter->get_output()))
            node->rebuild = do_rebuild;
          else
            node->rebuild = no_rebuild;
        }
      }

      /////////////////////////////////////////////////////
      // Rebuild the chain if nessesary

      if (node->rebuild == do_rebuild || node->filter->new_stream())
      {
        node->state = state_rebuild;

        // flush downstream if nessesary
        if (node->next->state != state_init)
        {
          node->next->flushing = true;
          node = node->next;
        }
        continue;
      }

      /////////////////////////////////////////////////////
      // Now we can process the data downstream 

      node->state = state_processing;
      node->next->input = node->output;
      node = node->next;
      continue;

    case state_rebuild:
      /////////////////////////////////////////////////////
      // Downstream was flushed. Update the chain's tail
      // and send the data of the new format (remains from
      // the processing state) down.

      if (!build_chain(node))
        throw FilterError(this, 0, "Error: cannot rebuild the chain");

      is_new_stream = true;
      node->state = state_processing;
      node->rebuild = no_rebuild;
      node->next->input = node->output;
      node = node->next;
      continue;

    case state_done_flushing:
      /////////////////////////////////////////////////////
      // Downstream was done flushing. Just go up to get
      // more data.

      node->state = state_init;
      node = node->prev;
      continue;
    }
  }

  return false;
}

bool
FilterGraph2::process(Chunk2 &in, Chunk2 &out)
{
  if (!chain_is_empty())
    if (process_chain(out))
      return true;

  if (in.is_dummy())
    return false;

  start.input = in;
  in.set_empty();
  return process_chain(out);
}

bool
FilterGraph2::flush(Chunk2 &out)
{
  start.flushing = true;
  return process_chain(out);
}

void
FilterGraph2::reset()
{
  is_new_stream = false;
  for (Node *node = &start; node; node = node->next)
  {
    node->filter->reset();
    node->state = state_init;
    node->flushing = false;
  }
}

///////////////////////////////////////////////////////////
// Truncate the chain
// truncate(node)  - truncate everything after the node
// truncate(start) - delete the whole chain

void
FilterGraph2::truncate(Node *node)
{
  node = node->next;
  while (node->id != node_end)
  {
    node->filter->close();
    uninit_filter(node->id);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    Node *next_node = node->next;
    delete node;
    node = next_node;
  }
}

///////////////////////////////////////////////////////////
// Rebuild the chain starting from the node given (first
// node does not change).
// Returns true on success and false otherwise.

bool
FilterGraph2::build_chain(Node *node)
{
  while (node->id != node_end)
  {
    Speakers next_spk = node->filter->get_output();

    /////////////////////////////////////////////////////////
    // if ofdd filter is in transition state then set output
    // format to spk_unknown

    if (next_spk == spk_unknown)
    {
      // ofdd = true;
      return true;
    }

    /////////////////////////////////////////////////////////
    // find the next node id

    int next_node_id = next_id(node->id, next_spk);
    if (next_node_id == node_err)
      return false;

    // truncate the chain if nessesary
    if (node->next->id != node_end)
      truncate(node);

    if (next_node_id == node_end)
    {
      // end of the chain
      node->next->filter->open(next_spk);
      node->next->state = state_init;
      node->next->rebuild = no_rebuild;
      node->next->flushing = false;
      return true;
    }

    /////////////////////////////////////////////////////////
    // Build a new node

    // create & init the filter
    Filter2 *filter = init_filter(next_node_id, next_spk);
    if (!filter)
      return false;

    if (!filter->open(next_spk))
    {
      uninit_filter(next_node_id);
      return false;
    }
    filter->reset();

    // build a new node
    Node *next_node   = new Node;
    next_node->id     = next_node_id;
    next_node->next   = &end;
    next_node->prev   = node;
    next_node->filter = filter;
    next_node->state  = state_init;
    next_node->rebuild = no_rebuild;
    next_node->flushing = false;

    // update the list
    node->next->prev = next_node;
    node->next = next_node;

    // go down
    node = node->next;
  }

  return true;
}

void
FilterGraph2::rebuild_node(int id)
{
  for (Node *node = &start; node->next; node = node->next)
    if (node->next->id == id)
    {
      node->rebuild = do_rebuild;
      return;
    }
}

void
FilterGraph2::invalidate()
{
  for (Node *node = &start; node; node = node->next)
    if (node->rebuild == no_rebuild)
      node->rebuild = check_rebuild;
}

void
FilterGraph2::destroy()
{
  truncate(&start);
  reset();
}

///////////////////////////////////////////////////////////////////////////////
// FilterChain
///////////////////////////////////////////////////////////////////////////////

FilterChain2::FilterChain2()
{
  id = 1;
  nodes.push_back(Node(node_start, 0));
  nodes.push_back(Node(node_end, 0));
}

FilterChain2::~FilterChain2()
{}

bool
FilterChain2::add_front(Filter2 *filter, const std::string &name)
{
  if (!filter || find(nodes.begin(), nodes.end(), filter) != nodes.end())
    return false;

  nodes.insert(++nodes.begin(), Node(id++, filter, name));
  invalidate();
  return true;
}

bool
FilterChain2::add_back(Filter2 *filter, const std::string &name)
{
  if (!filter || find(nodes.begin(), nodes.end(), filter) != nodes.end())
    return false;

  nodes.insert(--nodes.end(), Node(id++, filter, name));
  invalidate();
  return true;
}

void
FilterChain2::remove(Filter2 *filter)
{
  if (!filter) false;
  nodes.erase(find(nodes.begin(), nodes.end(), filter));
  invalidate();
}

void
FilterChain2::clear()
{
  nodes.clear();
  nodes.push_back(Node(node_start, 0));
  nodes.push_back(Node(node_end, 0));
  invalidate();
}

void
FilterChain2::destroy()
{
  nodes.clear();
  nodes.push_back(Node(node_start, 0));
  nodes.push_back(Node(node_end, 0));
  FilterGraph2::destroy();
}

int
FilterChain2::next_id(int id_, Speakers spk_) const
{
  const_list_iter it = ++find(nodes.begin(), nodes.end(), id_);
  if (it == nodes.end()) return node_err;
  return it->id;
}

Filter2 *
FilterChain2::init_filter(int id_, Speakers spk_)
{
  const_list_iter it = find(nodes.begin(), nodes.end(), id_);
  if (it == nodes.end()) return 0;
  return it->filter;
}

std::string
FilterChain2::get_name(int id_) const
{
  const_list_iter it = find(nodes.begin(), nodes.end(), id_);
  if (it == nodes.end()) return 0;
  return it->name.c_str();
}
